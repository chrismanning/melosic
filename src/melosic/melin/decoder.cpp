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

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

#include <melosic/common/optional.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/string.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/filecache.hpp>
#include <melosic/core/audiofile.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/melin/input.hpp>

#include "decoder.hpp"

namespace Melosic {
namespace Decoder {

using namespace TagLib;

Logger::Logger logject{logging::keywords::channel = "Decoder::Manager"};

class Manager::impl {
public:
    impl(Input::Manager& inman, Thread::Manager& tman) : inman(inman), tman(tman) {}
    Input::Manager& inman;
    Thread::Manager& tman;
    mutex mu;
    std::unordered_map<std::string, Factory> inputFactories;
    Core::FileCache m_file_cache;
};

Manager::Manager(Input::Manager& inman, Thread::Manager& tman) : pimpl(new impl(inman, tman)) {}

Manager::~Manager() {}

void Manager::addAudioFormat(Factory fact, boost::string_ref mime_type) {
    unique_lock l(pimpl->mu);

    auto mime = mime_type.to_string();
    boost::to_lower(mime);

    LOG(logject) << "Adding support for MIME type " << mime_type;
    auto bef = pimpl->inputFactories.size();

    auto pos = pimpl->inputFactories.find(mime);
    if(pos == pimpl->inputFactories.end()) {
        pimpl->inputFactories.emplace(std::move(mime), fact);
        assert(++bef == pimpl->inputFactories.size());
    }
    else {
        WARN_LOG(logject) << mime_type << ": already registered to a decoder factory";
    }
}

std::vector<Core::Track> Manager::tracks(const network::uri& uri) const {
    Core::Track t{uri};
    using namespace boost::literals;
    if(uri.scheme() == "file"_s_ref) {
        auto path = Input::uri_to_path(uri);
        FileRef taglib_file{path.c_str()};
        TRACE_LOG(logject) << "in tracks(uri); path: " << path;
        TRACE_LOG(logject) << "in tracks(uri); path.c_str(): " << path.c_str();
        if(taglib_file.isNull())
            return {};
        assert(taglib_file.tag() != nullptr);
        auto taglib_tags = taglib_file.tag()->properties();
        if(/*is playlist or cue*/taglib_tags.contains("CUEFILE"))
            return {};
        Core::TagMap tags;
        for(const auto& tag : taglib_tags) {
            for(const auto& v : tag.second)
                tags.emplace(tag.first.to8Bit(true), v.to8Bit(true));
        }
        t.tags(tags);

        auto ap = taglib_file.audioProperties();
        t.audioSpecs({(uint8_t)ap->channels(), 0, (uint32_t)ap->sampleRate()});
        t.end(chrono::seconds{ap->length()});
    }
    auto pcm_src = open(t);
    if(pcm_src) {
        t.end(pcm_src->duration());
        t.audioSpecs(pcm_src->getAudioSpecs());
    }
    return {t};
}

struct libmagic_handle final {
    libmagic_handle() noexcept : libmagic_handle(MAGIC_NONE) {}

    explicit libmagic_handle(int flags) noexcept {
        m_handle = magic_open(flags);
        assert(m_handle != nullptr);
        int r = magic_load(m_handle, nullptr);
        assert(r == 0);
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

static std::string detect_mime_type(const network::uri& uri) {
    using namespace boost::literals;
    thread_local libmagic_handle cookie(MAGIC_SYMLINK|MAGIC_MIME_TYPE|MAGIC_ERROR);

    if(uri.scheme() == "file"_s_ref) {
        auto p = Input::uri_to_path(uri);
        TRACE_LOG(logject) << "Detecting MIME for file " << p;
        if(auto str = magic_file(cookie, p.c_str())) {
            TRACE_LOG(logject) << "Detected MIME " << str << " for file " << p;
            return {str};
        }
    }
    TRACE_LOG(logject) << "Could not detect MIME for " << uri;
    return {};
}

std::unique_ptr<PCMSource> Manager::open(const Core::Track& track) const {
    unique_lock l(pimpl->mu);
    // TODO: detect mime type here
    std::string mime_type = detect_mime_type(track.uri());
    if(mime_type.empty())
        return nullptr;
    auto is = pimpl->inman.open(track.uri());
    if(!is)
        return nullptr;

    TRACE_LOG(logject) << "Trying to open file with MIME " << mime_type;
    auto fact = pimpl->inputFactories.find(mime_type);

    std::unique_ptr<PCMSource> ret;
    if(fact != pimpl->inputFactories.end())
        ret = fact->second(std::move(is));

    if(ret && track.start() != 0ms)
        ret->seek(track.start());
    return std::move(ret);
}

} // namespace Decoder
} // namespace Melosic
