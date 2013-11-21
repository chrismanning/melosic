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
#include <boost/locale/collator.hpp>
#include <boost/thread/synchronized_value.hpp>

#include <taglib/tpropertymap.h>
#include <taglib/tfile.h>
#include <taglib/fileref.h>

#include "track.hpp"
#include <melosic/common/common.hpp>
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

class Track::impl {
public:
    impl(boost::filesystem::path filename,
         chrono::milliseconds start, chrono::milliseconds end)
        : filepath(std::move(filename)), start(start), end(end)
    {}

    ~impl() {}

    chrono::milliseconds duration() {
        if(end > start)
            return end - start;
        return 0ms;
    }

    optional<std::string> getTag(const std::string& key) {
        auto tags = m_tags.synchronize();
        auto it = tags->find(key);
        if(it == tags->end() && key.substr(0, 5) == "album" && key != "albumtitle" && key.size() > 5)
            it = tags->find(key.substr(5));

        return (it != tags->end()) ? optional<std::string>{it->second} : nullopt;
    }

    std::string getTagOr(const std::string& key) {
        auto t = getTag(key);
        return t ? *t : "?"s;
    }

//    void reloadTags(unique_lock& l) {
//        reloadTags();
//        scope_unlock_exit_lock<unique_lock> s{l};
//        tagsChanged(std::cref(tags));
//    }

//    void reloadTags() {
//        tags.clear();
//        taglibfile.reset(TagLib::FileRef::create(filepath.string().c_str()));
//        if(!taglibfile)
//            return;
//        tags = taglibfile->properties();
//        taglibfile.reset();
//        m_tags_readable = true;
//        TRACE_LOG(logject) << tags;
//    }

//    bool taggable() {
//        return static_cast<bool>(taglibfile);
//    }

    const boost::filesystem::path& filePath() {
        return filepath;
    }

    optional<std::string> parse_format(std::string, boost::system::error_code&);

private:
    friend class Track;
    boost::filesystem::path filepath;
    chrono::milliseconds start, end;
    boost::synchronized_value<TagMap> m_tags;
    AudioSpecs as;
    TagsChanged tagsChanged;
    mutex mu;
};

Track::Track(boost::filesystem::path filename,
             chrono::milliseconds end,
             chrono::milliseconds start)
    : pimpl(std::make_shared<impl>(std::move(filename), start, end))
{}

Track::~Track() {}

bool Track::operator==(const Track& b) const noexcept {
    return pimpl == b.pimpl;
}

bool Track::operator!=(const Track& b) const noexcept {
    return pimpl != b.pimpl;
}

void Track::setAudioSpecs(AudioSpecs as) {
    unique_lock l(pimpl->mu);
    pimpl->as = as;
}

void Track::setTimePoints(chrono::milliseconds end, chrono::milliseconds start) {
    lock_guard l(pimpl->mu);
    pimpl->start = start;
    pimpl->end = end;
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

const boost::filesystem::path& Track::filePath() const {
    shared_lock l(pimpl->mu);
    return pimpl->filePath();
}

boost::synchronized_value<TagMap>& Track::getTags() {
    unique_lock l(pimpl->mu);
    return pimpl->m_tags;
}

const boost::synchronized_value<TagMap>& Track::getTags() const {
    unique_lock l(pimpl->mu);
    return pimpl->m_tags;
}

void Track::setTags(TagMap tags) {
    {
        lock_guard l(pimpl->mu);
        pimpl->m_tags = std::move(tags);
    }
    pimpl->tagsChanged(std::cref(pimpl->m_tags));
}

//void Track::reloadTags() {
//    unique_lock l(pimpl->mu);
//    pimpl->reloadTags(l);
//}

//bool Track::taggable() const {
//    shared_lock l(pimpl->mu);
//    return pimpl->taggable();
//}

bool Track::tagsReadable() const {
    shared_lock l(pimpl->mu);
    return !pimpl->m_tags->empty();
}

optional<std::string> Track::format_string(const std::string& fmt_str) const {
    lock_guard l(pimpl->mu);
    boost::system::error_code ec;
    return pimpl->parse_format(fmt_str, ec);
}

Signals::Track::TagsChanged& Track::getTagsChangedSignal() const noexcept {
    return pimpl->tagsChanged;
}

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

size_t hash_value(const Track& b) {
    std::hash<std::shared_ptr<Track::impl>> hasher;
    return hasher(b.pimpl);
}

} // namespace Core
} // namespace Melosic
