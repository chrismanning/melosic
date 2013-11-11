/**************************************************************************
**  Copyright (C) 2012 Christian Manning
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

#include <thread>
#include <mutex>

#include <boost/thread/shared_mutex.hpp>
using mutex = boost::shared_mutex;
using shared_lock = boost::shared_lock<mutex>;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <taglib/tpropertymap.h>
#include <taglib/tfile.h>
#include <taglib/fileref.h>

#include "track.hpp"
#include <melosic/common/stream.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/common/optional.hpp>

namespace Melosic {
namespace Core {

static Logger::Logger logject(logging::keywords::channel = "Track");

struct TagsChanged : Signals::Signal<Signals::Track::TagsChanged> {
    using Super::Signal;
};

class Track::impl : public std::enable_shared_from_this<impl> {
public:
    impl(Decoder::Factory factory, boost::filesystem::path filename,
         chrono::milliseconds start, chrono::milliseconds end)
        : filepath(std::move(filename)), decoder_factory(std::move(factory)), start(start), end(end)
    {
//        open(filename, start, end);
    }

    ~impl() {}

    void open(boost::filesystem::path filename, chrono::milliseconds start, chrono::milliseconds end) {
        using std::swap;

        close();
        swap(filepath, filename);
        swap(this->start, start);
        swap(this->end, end);
        input.open(filepath);
        reloadDecoder();
        reloadTags();
        seek(0ms);
    }

    bool isOpen() {
        return input.isOpen() && static_cast<bool>(decoder);
    }

    void reOpen() {
        open(filepath, start, end);
    }

    void close() {
        taglibfile.reset();
        decoder.reset();
        input.close();
    }

    std::streamsize read(char * s, std::streamsize n) {
        if(!isOpen())
            reOpen();
        try {
//            if(duration() == decoder->duration()) {
//                auto difference = (end > start) ? +std::llrint((end - decoder->tell()).count() *
//                                                               (as.sample_rate/1000.0) *
//                                                               (as.bps/8)) : 0;
//                n = difference > n ? n : +difference;
//            }
            return decoder->read(s, n);
        }
        catch(AudioDataInvalidException& e) {
            e << ErrorTag::FilePath(filePath());
            throw;
        }
    }

    void seek(chrono::milliseconds dur) {
        if(!isOpen())
            reOpen();
        decoder->seek(dur + start);
    }

    chrono::milliseconds tell() {
        if(!isOpen())
            reOpen();
        return decoder->tell() - start;
    }

    void reset() {
        if(isOpen())
            decoder->reset();
    }

    chrono::milliseconds duration() {
        if(end > start)
            return end - start;
        return 0ms;
    }

    optional<std::string> getTag(const std::string& key) {
        TagLib::String key_(key.c_str());
        auto val = tags.find(key_);
        if(val == tags.end() && key.substr(0, 5) == "album" && key != "albumtitle" && key.size() > 5)
            val = tags.find(TagLib::String(key.substr(5).c_str()));

        return val != tags.end() ? optional<std::string>{val->second.front().to8Bit(true)} : nullopt;
    }

    std::string getTagOr(const std::string& key) {
        auto t = getTag(key);
        return t ? *t : "?"s;
    }

    void reloadDecoder() {
        try {
            decoder = decoder_factory(input);
            if(as == decoder->getAudioSpecs())
                decoder->getAudioSpecs() = as;
            else
                as = decoder->getAudioSpecs();
            if(end <= start)
                end = start + decoder->duration();
        }
        catch(DecoderException& e) {
            e << ErrorTag::FilePath(input.filename());
            throw;
        }
    }

    bool decodable() {
        return static_cast<bool>(decoder_factory);
    }

    void reloadTags() {
        tags.clear();
        taglibfile.reset(TagLib::FileRef::create(filepath.string().c_str()));
        if(!taglibfile)
            return;
        tags = taglibfile->properties();
        taglibfile.reset();
        m_tags_readable = true;
        tagsChanged(std::cref(tags));
        TRACE_LOG(logject) << tags.toString().to8Bit(true);
    }

    bool taggable() {
        return static_cast<bool>(taglibfile);
    }

    bool valid() {
        return decoder_factory && decoder && *decoder;
    }

    const boost::filesystem::path& filePath() {
        return filepath;
    }

    std::shared_ptr<impl> clone() {
        return std::make_shared<impl>(decoder_factory, filepath, start, end);
    }

    optional<std::string> parse_format(std::string, boost::system::error_code&);

private:
    friend class Track;
    boost::filesystem::path filepath;
    Decoder::Factory decoder_factory;
    IO::File input;
    chrono::milliseconds start, end;
    std::unique_ptr<Decoder::Playable> decoder;
    std::unique_ptr<TagLib::File> taglibfile;
    TagLib::PropertyMap tags;
    AudioSpecs as;
    bool m_tags_readable{false};
    TagsChanged tagsChanged;
    mutex mu;
};

Track::Track(Decoder::Factory factory,
             boost::filesystem::path filename,
             chrono::milliseconds start,
             chrono::milliseconds end)
    : pimpl(std::make_shared<impl>(std::move(factory),
                                   std::move(filename),
                                   start, end))
{}

Track::~Track() {}

bool Track::operator==(const Track& b) const noexcept {
    return pimpl == b.pimpl;
}

bool Track::operator!=(const Track& b) const noexcept {
    return pimpl != b.pimpl;
}

void Track::setTimePoints(chrono::milliseconds start, chrono::milliseconds end) {
    lock_guard l(pimpl->mu);
    pimpl->start = start;
    pimpl->end = end;
}

std::streamsize Track::read(char * s, std::streamsize n) {
    lock_guard l(pimpl->mu);
    return pimpl->read(s, n);
}

void Track::close() {
    lock_guard l(pimpl->mu);
    pimpl->close();
}

bool Track::isOpen() const {
    shared_lock l(pimpl->mu);
    return pimpl->isOpen();
}

void Track::reOpen() {
    lock_guard l(pimpl->mu);
    pimpl->reOpen();
}

void Track::seek(chrono::milliseconds dur) {
    lock_guard l(pimpl->mu);
    pimpl->seek(dur);
}

chrono::milliseconds Track::tell() {
    shared_lock l(pimpl->mu);
    return pimpl->tell();
}

void Track::reset() {
    lock_guard l(pimpl->mu);
    pimpl->reset();
}

chrono::milliseconds Track::duration() const {
    shared_lock l(pimpl->mu);
    return pimpl->duration();
}

Melosic::AudioSpecs Track::getAudioSpecs() const {
    shared_lock l(pimpl->mu);
    return pimpl->as;
}

optional<std::string> Track::getTag(const std::string& key) const {
    shared_lock l(pimpl->mu);
    return pimpl->getTag(key);
}

Track::operator bool() const {
    shared_lock l(pimpl->mu);
    return pimpl->valid();
}

const boost::filesystem::path& Track::filePath() const {
    shared_lock l(pimpl->mu);
    return pimpl->filePath();
}

void Track::reloadTags() {
    unique_lock l(pimpl->mu);
    pimpl->reloadTags();
}

bool Track::taggable() const {
    shared_lock l(pimpl->mu);
    return pimpl->taggable();
}

bool Track::tagsReadable() const {
    shared_lock l(pimpl->mu);
    return pimpl->m_tags_readable;
}

void Track::reloadDecoder() {
    lock_guard l(pimpl->mu);
    pimpl->reloadDecoder();
}

bool Track::decodable() const {
    shared_lock l(pimpl->mu);
    return pimpl->decodable();
}

bool Track::valid() const {
    shared_lock l(pimpl->mu);
    return pimpl->valid();
}

Track Track::clone() const {
    lock_guard l(pimpl->mu);
    return Track(pimpl->clone());
}

optional<std::string> Track::format_string(const std::string& fmt_str) const {
    lock_guard l(pimpl->mu);
    boost::system::error_code ec;
    return pimpl->parse_format(fmt_str, ec);
}

Signals::Track::TagsChanged& Track::getTagsChangedSignal() const noexcept {
    return pimpl->tagsChanged;
}

Track::Track(decltype(pimpl) p) : pimpl(p) {}

optional<std::string> Track::impl::parse_format(std::string str, boost::system::error_code&) {
    if(str.empty())
        return nullopt;
    namespace qi = boost::spirit::qi;
    namespace phx = boost::phoenix;
    qi::rule<std::string::iterator, std::string()> long_field = '{' > +(qi::char_ - qi::char_("\n{}")) > '}';
    qi::symbols<char, std::string> short_field;
    short_field.add("a", "artist")
                   ("A", "albumartist")
                   ("t", "title")
                   ("T", "album")
                   ("n", "tracknumber")
                   ("N", "totaltracks")
                   ("d", "date")
                   ("g", "genre")
                   ("c", "comment")
                   ("f", "file")
                   ("p", "filepath")
                   ("e", "extension");

    qi::rule<std::string::iterator, std::string()> tag = qi::lit('%') > (long_field | short_field) [
            qi::_val = phx::bind(&Track::impl::getTagOr, this, qi::_1)
    ];

    std::vector<std::string> strs;
    auto r = qi::parse(str.begin(), str.end(), *(tag | +(qi::char_-'%')), strs);
    assert(r);
    std::stringstream strm;
    for(const auto& v : strs)
        strm << v;
    if(!strm.str().empty())
        return strm.str();
    return nullopt;
}

} // namespace Core
} // namespace Melosic
