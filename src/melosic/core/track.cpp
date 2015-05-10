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

#include <shared_mutex>
using mutex = std::shared_timed_mutex;
using shared_lock = std::shared_lock<mutex>;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/thread/synchronized_value.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <jbson/document.hpp>
#include <jbson/builder.hpp>

#include "track.hpp"
#include <melosic/common/common.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/common/signal.hpp>

namespace Melosic {
namespace Core {

static Logger::Logger logject(logging::keywords::channel = "Track");

struct TagsChanged : Signals::Signal<Signals::Track::TagsChanged> {
    using Super::Signal;
};

class Track::impl {
  public:
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

    const network::uri& uri() {
        return m_uri;
    }

    optional<std::string> parse_format(std::string, std::error_code&);

  private:
    friend class Track;
    network::uri m_uri;
    chrono::milliseconds start{0}, end{0};
    boost::synchronized_value<TagMap> m_tags;
    AudioSpecs as;
    TagsChanged tagsChanged;
    mutex mu;
};

Track::Track(const network::uri& location, chrono::milliseconds end, chrono::milliseconds start)
    : pimpl(std::make_shared<impl>()) {
    pimpl->m_uri = location;
    pimpl->end = end;
    pimpl->start = start;
}

Track::Track(const jbson::document& track_doc) : pimpl(std::make_shared<impl>()) {
    auto it = track_doc.find("type");
    if(it == track_doc.end() || it->type() != jbson::element_type::string_element ||
       jbson::get<jbson::element_type::string_element>(*it) != "track") {
        BOOST_THROW_EXCEPTION(std::runtime_error("document is not a track"));
    }

    using jbson::element_type;
    for(auto&& element : track_doc) {
        if(element.name() == "location") {
            try {
                pimpl->m_uri = network::uri(element.value<std::string>());
            } catch(...) {
                std::throw_with_nested(std::runtime_error("'location' should be a string"));
            }
        } else if(element.name() == "start") {
            if(element.type() == element_type::int32_element)
                pimpl->start = chrono::milliseconds{element.value<int32_t>()};
            else if(element.type() == element_type::int64_element)
                pimpl->start = chrono::milliseconds{element.value<int64_t>()};
            else
                BOOST_THROW_EXCEPTION(std::runtime_error("'start' should be an integer"));
        } else if(element.name() == "end") {
            if(element.type() == element_type::int32_element)
                pimpl->end = chrono::milliseconds{element.value<int32_t>()};
            else if(element.type() == element_type::int64_element)
                pimpl->end = chrono::milliseconds{element.value<int64_t>()};
            else
                BOOST_THROW_EXCEPTION(std::runtime_error("'end' should be an integer"));
        } else if(element.name() == "metadata") {
            TagMap tags;

            decltype(jbson::get<element_type::array_element>(element)) metadata;
            try {
                metadata = jbson::get<element_type::array_element>(element);
            } catch(...) {
                std::throw_with_nested(std::runtime_error("'metadata' should be an array"));
            }

            for(auto&& tag : metadata) {
                decltype(jbson::get<element_type::document_element>(tag)) tag_doc;
                try {
                    tag_doc = jbson::get<element_type::document_element>(tag);
                } catch(...) {
                    std::throw_with_nested(std::runtime_error("'metadata' elements should all be document"));
                }

                std::pair<std::string, std::string> pair;
                auto it = tag_doc.find("key");
                if(it == tag_doc.end() || it->type() != element_type::string_element)
                    BOOST_THROW_EXCEPTION(std::runtime_error("tag must have key"));
                pair.first = it->value<std::string>();

                it = tag_doc.find("value");
                if(it == tag_doc.end() || it->type() != element_type::string_element)
                    BOOST_THROW_EXCEPTION(std::runtime_error("tag must have value"));
                pair.second = it->value<std::string>();

                tags.emplace(std::move(pair));
            }
            pimpl->m_tags = boost::synchronized_value<TagMap>(std::move(tags));
        }
    }

    if(pimpl->m_uri.empty())
        BOOST_THROW_EXCEPTION(std::runtime_error("track must have a location"));
}

Track::~Track() {
}

bool Track::operator==(const Track& b) const noexcept {
    return pimpl == b.pimpl ||
           (pimpl->m_uri == b.pimpl->m_uri && pimpl->end == b.pimpl->end && pimpl->start == b.pimpl->start);
}

bool Track::operator!=(const Track& b) const noexcept {
    return !(*this == b);
}

void Track::audioSpecs(AudioSpecs as) {
    unique_lock l(pimpl->mu);
    pimpl->as = as;
}

void Track::start(chrono::milliseconds start) {
    lock_guard l(pimpl->mu);
    pimpl->start = start;
}

void Track::end(chrono::milliseconds end) {
    lock_guard l(pimpl->mu);
    pimpl->end = end;
}

chrono::milliseconds Track::start() const {
    shared_lock l(pimpl->mu);
    return pimpl->start;
}

chrono::milliseconds Track::end() const {
    shared_lock l(pimpl->mu);
    return pimpl->end;
}

chrono::milliseconds Track::duration() const {
    shared_lock l(pimpl->mu);
    return pimpl->duration();
}

Melosic::AudioSpecs Track::audioSpecs() const {
    shared_lock l(pimpl->mu);
    return pimpl->as;
}

optional<std::string> Track::tag(const std::string& key) const {
    shared_lock l(pimpl->mu);
    return pimpl->getTag(key);
}

const network::uri& Track::uri() const {
    shared_lock l(pimpl->mu);
    return pimpl->m_uri;
}

boost::synchronized_value<TagMap>& Track::tags() {
    unique_lock l(pimpl->mu);
    return pimpl->m_tags;
}

const boost::synchronized_value<TagMap>& Track::tags() const {
    unique_lock l(pimpl->mu);
    return pimpl->m_tags;
}

void Track::tags(TagMap tags) {
    {
        lock_guard l(pimpl->mu);
        pimpl->m_tags = std::move(tags);
    }
    pimpl->tagsChanged(std::cref(pimpl->m_tags));
}

// void Track::reloadTags() {
//    unique_lock l(pimpl->mu);
//    pimpl->reloadTags(l);
//}

// bool Track::taggable() const {
//    shared_lock l(pimpl->mu);
//    return pimpl->taggable();
//}

bool Track::tagsReadable() const {
    shared_lock l(pimpl->mu);
    return !pimpl->m_tags->empty();
}

optional<std::string> Track::format_string(const std::string& fmt_str) const {
    lock_guard l(pimpl->mu);
    std::error_code ec;
    return pimpl->parse_format(fmt_str, ec);
}

Signals::Track::TagsChanged& Track::getTagsChangedSignal() const noexcept {
    return pimpl->tagsChanged;
}

jbson::document Track::bson() const {
    using std::get;
    using jbson::element_type;

    shared_lock l(pimpl->mu);

    auto ob = jbson::builder("type", "track")("location", element_type::string_element, uri().string())(
        "channels", element_type::int32_element, pimpl->as.channels)("sample rate", element_type::int64_element,
                                                                     pimpl->as.sample_rate)(
        "start", element_type::int64_element, pimpl->start.count())("end", element_type::int64_element,
                                                                    pimpl->end.count());
    auto arr = pimpl->m_tags([](auto&& metadata) {
        jbson::array_builder arb;
        for(auto&& pair : metadata)
            arb(jbson::element_type::document_element,
                jbson::builder("key", element_type::string_element, boost::to_lower_copy(get<0>(pair)))(
                    "value", element_type::string_element, get<1>(pair)));
        return std::move(arb);
    });
    ob("metadata", element_type::array_element, arr);

    return ob;
}

bool operator<(const Track& a, const Track& b) {
#ifdef _GNU_SOURCE
    auto a_str = a.uri().string(), b_str = b.uri().string();
    auto cmp = ::strverscmp(a_str.c_str(), b_str.c_str());
#else
    auto cmp = a.uri().compare(b.uri());
#endif
    if(cmp == 0)
        return a.start() < b.start();
    return cmp < 0;
}

optional<std::string> Track::impl::parse_format(std::string str, std::error_code&) {
    if(str.empty())
        return nullopt;
    namespace qi = boost::spirit::qi;
    namespace phx = boost::phoenix;
    qi::rule<std::string::iterator, std::string()> long_field = '{' > +(qi::char_ - qi::char_("\n{}")) > '}';
    qi::symbols<char, std::string> short_field;
    short_field.add("a", "artist")("A", "albumartist")("t", "title")("T", "album")("n", "tracknumber")(
        "N", "totaltracks")("d", "date")("g", "genre")("c", "comment")("f", "file")("p", "filepath")("e", "extension");

    qi::rule<std::string::iterator, std::string()> tag =
        qi::lit('%') > (long_field | short_field)[qi::_val = phx::bind(&Track::impl::getTagOr, this, qi::_1)];

    std::vector<std::string> strs;
    auto r = qi::parse(str.begin(), str.end(), *(tag | +(qi::char_ - '%')), strs);
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
