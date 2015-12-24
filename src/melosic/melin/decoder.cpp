/**************************************************************************
**  Copyright (C) 2013 Christian Manning
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <magic.h>

#include <unordered_map>
#include <string>
#include <functional>
namespace ph = std::placeholders;
#include <mutex>
using mutex = std::mutex;
using unique_lock = std::unique_lock<mutex>;

#include <boost/range/adaptor/map.hpp>
using namespace boost::adaptors;
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/filesystem/fstream.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <asio/error.hpp>

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

#include <melosic/common/optional.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/string.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/filecache.hpp>
#include <melosic/core/audiofile.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/common/pcmbuffer.hpp>
#include <melosic/common/typeid.hpp>

#include "decoder.hpp"

namespace Melosic {
namespace Decoder {

using namespace TagLib;

Logger::Logger logject{logging::keywords::channel = "Decoder::Manager"};

struct Manager::impl : std::enable_shared_from_this<impl> {
    impl(const std::shared_ptr<Input::Manager>& inman, const std::shared_ptr<Plugin::Manager>& plugman)
        : inman(inman), plugman(plugman) {
    }
    std::shared_ptr<Input::Manager> inman;
    std::shared_ptr<Plugin::Manager> plugman;
    mutex mu;
    std::unordered_map<std::string, Factory> inputFactories;
    Core::FileCache m_file_cache;
    std::vector<std::shared_ptr<provider>> providers;

    std::unique_ptr<PCMSource> open(const network::uri&);
};

Manager::Manager(const std::shared_ptr<Input::Manager>& inman, const std::shared_ptr<Plugin::Manager>& plugman)
    : pimpl(std::make_shared<impl>(inman, plugman)) {
}

Manager::~Manager() {
}

void Manager::add_provider(std::shared_ptr<provider> provider) {
    unique_lock l(pimpl->mu);
    auto& ref = *provider.get();
    TRACE_LOG(logject) << "Adding provider " << typeid(ref);
    pimpl->providers.emplace_back(std::move(provider));
}

std::vector<Core::Track> Manager::tracks(const network::uri& uri) const {
    try {
        if(uri.scheme() && uri.scheme()->to_string() == "file")
            return tracks(Input::uri_to_path(uri));
    } catch(network::percent_decoding_error e) {
        ERROR_LOG(logject) << "Error decoding uri (" << uri << "): " << e.what();
        DEBUG_LOG(logject) << boost::diagnostic_information(e);
    }

    return {};
}

std::vector<Core::Track> Manager::tracks(const fs::path& path) const {
    std::vector<Core::Track> ret;

    boost::system::error_code ec;
    const auto status = fs::status(path, ec);
    if(ec) {
        ERROR_LOG(logject) << "Could not get tracks for path (" << path << "): " << ec.message();
        return {};
    }

    if(fs::is_directory(status)) {
        for(auto&& entry : fs::recursive_directory_iterator(path))
            boost::range::push_back(ret, tracks(entry.path()));
        std::sort(ret.begin(), ret.end());
    } else if(fs::is_regular_file(status)) {
        // TODO: detect files, including playlists
        FileRef taglib_file{path.c_str()};
        if(taglib_file.isNull())
            return ret;

        assert(taglib_file.tag() != nullptr);
        auto taglib_tags = taglib_file.tag()->properties();
        if(/*is playlist or cue*/ taglib_tags.contains("CUEFILE"))
            return ret;

        Core::TagMap tags;
        for(const auto& tag : taglib_tags) {
            for(const auto& v : tag.second)
                tags.emplace(tag.first.to8Bit(true), v.to8Bit(true));
        }
        Core::Track t{Input::to_uri(path)};
        t.tags(tags);

        auto ap = taglib_file.audioProperties();
        t.audioSpecs({static_cast<uint8_t>(ap->channels()), 0, static_cast<uint32_t>(ap->sampleRate())});
        t.end(chrono::seconds{ap->length()});

        try {
            auto pcm_src = pimpl->open(t.uri());
            if(pcm_src) {
                t.end(pcm_src->duration());
                t.audioSpecs(pcm_src->getAudioSpecs());
            }
        } catch(...) {
            ERROR_LOG(logject) << "Could not open track at uri " << t.uri();
            ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
        }

        ret.push_back(std::move(t));
    }

    return ret;
}

struct libmagic_handle final {
    libmagic_handle() noexcept : libmagic_handle(MAGIC_NONE) {
    }

    explicit libmagic_handle(int flags) noexcept {
        m_handle = magic_open(flags);
        assert(m_handle != nullptr);
        int r = magic_load(m_handle, nullptr);
        assert(r == 0);
        (void)r;
    }

    ~libmagic_handle() noexcept {
        magic_close(m_handle);
    }

    operator magic_t() const noexcept {
        return m_handle;
    }

  private:
    magic_t m_handle;
};

static std::optional<std::string> detect_mime_type(std::istream& stream) {
    thread_local libmagic_handle cookie(MAGIC_SYMLINK | MAGIC_MIME_TYPE | MAGIC_ERROR);

    std::array<char, 80> buf;
    auto cur = stream.tellg();
    stream.read(buf.data(), buf.size());
    stream.seekg(cur);

    if(auto mime = magic_buffer(cookie, buf.data(), buf.size())) {
        TRACE_LOG(logject) << "Detected MIME " << mime << " for stream";
        return {mime};
    }

    TRACE_LOG(logject) << "Could not detect MIME for stream";
    return std::nullopt;
}

struct TrackSource : PCMSource {
    explicit TrackSource(std::unique_ptr<PCMSource> impl, chrono::milliseconds start, chrono::milliseconds end)
        : pimpl(std::move(impl)), m_start(start), m_end(end > m_start ? end : pimpl->duration()) {
        TRACE_LOG(logject) << "TrackSource created; start: " << m_start.count() << "ms; end: " << m_end.count() << "ms";
        assert(start >= 0ms);
        assert(end >= 0ms);
        if(start > 0ms)
            pimpl->seek(start);
        //        if(m_end != pimpl->duration() && +(m_end - pimpl->duration()) < 1000ms)
        //            m_end = pimpl->duration();
    }

    virtual ~TrackSource() {
    }

    void seek(chrono::milliseconds dur) override {
        dur += m_start;
        if(dur >= m_end)
            dur = pimpl->duration();
        pimpl->seek(dur);
    }
    chrono::milliseconds tell() const override {
        return std::max(pimpl->tell() - m_start, m_start);
    }
    chrono::milliseconds duration() const override {
        if(m_end <= m_start)
            return pimpl->duration();
        return m_end - m_start;
    }
    AudioSpecs getAudioSpecs() const override {
        return pimpl->getAudioSpecs();
    }
    size_t decode(PCMBuffer& buf, std::error_code& ec) override {
        if(pimpl->tell() >= m_end) {
            ec = asio::error::eof;
            return 0;
        }
        auto bytes = pimpl->decode(buf, ec);
        TRACE_LOG(logject) << "Decoded " << bytes << " bytes";
        auto time = pimpl->tell() + getAudioSpecs().bytes_to_time<std::chrono::milliseconds>(bytes);
        if(time > m_end) {
            TRACE_LOG(logject) << "time: " << time.count() << "ms; m_end: " << m_end.count() << "ms";
            bytes -= getAudioSpecs().time_to_bytes(time - m_end);
        }
        TRACE_LOG(logject) << "Decoded " << bytes << " bytes; adjusted for time";
        return bytes;
    }
    bool valid() const override {
        return pimpl->valid() && (tell() < duration() && tell() >= 0ms);
    }
    void reset() override {
        pimpl->reset();
    }

  private:
    std::unique_ptr<PCMSource> pimpl;
    const chrono::milliseconds m_start;
    chrono::milliseconds m_end;
};

std::unique_ptr<PCMSource> Manager::impl::open(const network::uri& uri) {
    unique_lock l(mu);
    DEBUG_LOG(logject) << "Attempting to open a stream for " << uri;

    if(auto is = inman->open(uri)) {
        if(auto mime_type = detect_mime_type(*is)) {
            TRACE_LOG(logject) << "Attempting to decode stream with MIME " << *mime_type;
            auto predicate = [&](const auto& provider) { return provider->supports_mime(*mime_type); };
            try {
                for(const auto& provider : providers | boost::adaptors::filtered(predicate)) {
                    return provider->make_decoder(std::move(is));
                }
            } catch(...) {
                ERROR_LOG(logject) << "Error creating decoder: " << boost::current_exception_diagnostic_information();
            }
            ERROR_LOG(logject) << "No decoders could open stream with MIME " << *mime_type << " at " << uri;
        }
    }

    return nullptr;
}

std::unique_ptr<PCMSource> Manager::open(const Core::Track& track) const {
    auto ret = pimpl->open(track.uri());
    //    if(ret)
    //        ret = std::make_unique<TrackSource>(std::move(ret), track.start(), track.end());
    return ret;
}

template <typename Fun>
static auto decode_all(const std::unique_ptr<PCMSource>& source, Fun&& fun) {
    std::error_code ec;
    std::array<char, 1024> data{{0}};
    while(!ec) {
        Melosic::PCMBuffer buf{data.data(), data.size()};
        auto n = source->decode(buf, ec);
        assert(n <= data.size());
        fun(data.begin(), std::next(data.begin(), n));
    }
    return ec;
}

std::array<unsigned char, MD5_DIGEST_LENGTH> get_pcm_md5(std::unique_ptr<PCMSource> source) {
    std::array<unsigned char, MD5_DIGEST_LENGTH> checksum{{0}};
    MD5_CTX ctx;
    if(!MD5_Init(&ctx)) {
        assert(false);
    }
    std::error_code ec = decode_all(source, [&](auto first, auto last) {
        if(!MD5_Update(&ctx, first, std::distance(first, last))) {
            assert(false);
        }
    });
    MD5_Final(checksum.data(), &ctx);

    return checksum;
}

} // namespace Decoder
} // namespace Melosic
