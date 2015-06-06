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

#ifndef LASTFM_ARTIST_HPP
#define LASTFM_ARTIST_HPP

#include <string>
#include <experimental/string_view>
#include <experimental/type_traits>
#include <future>
#include <chrono>
#include <experimental/optional>

#include <boost/thread/future.hpp>

#include <network/uri.hpp>

#include <jbson/element.hpp>

#include "lastfm.hpp"
#include "wiki.hpp"
#include "tag.hpp"

namespace std {

template <typename Container, typename Elem>
void value_get(const jbson::basic_element<Container>& vector_elem, std::vector<Elem>& var) {
    auto arr = jbson::get<jbson::element_type::array_element>(vector_elem);
    for(auto&& elem : arr) {
        var.push_back(jbson::get<Elem>(elem));
    }
}
}

namespace lastfm {
class service;
struct tag;

struct LASTFM_EXPORT artist {
    explicit artist() = default;

    std::string_view name() const;
    void name(std::string_view);

    const network::uri& url() const;
    void url(const network::uri&);

    const std::vector<artist>& similar() const;
    void similar(std::vector<artist>);

    const std::vector<tag>& tags() const;
    void tags(std::vector<tag>);

    int listeners() const;
    void listeners(int);

    int plays() const;
    void plays(int);

    bool streamable() const;
    void streamable(bool);

    const wiki_t& wiki() const;
    void wiki(wiki_t);

    // api methods

    static std::future<artist> get_info(service&, std::string_view name);
    std::future<artist> get_info(service&) const;

    std::future<std::vector<artist>> get_similar(service&) const;

  private:
    std::string m_name;
    network::uri m_url;
    std::vector<artist> m_similar;
    std::vector<tag> m_tags;
    int m_listeners = 0;
    int m_plays = 0;
    bool m_streamable = false;
    wiki_t m_wiki;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& artist_elem, artist& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(artist_elem);
    for(auto&& elem : doc) {
        if(elem.name() == "name") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.name({str.data(), str.size()});
        } else if(elem.name() == "url") {
            var.url(jbson::get<network::uri>(elem));
        } else if(elem.name() == "listeners") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.listeners(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "plays") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.plays(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "streamable") {
            var.streamable(jbson::get<jbson::element_type::string_element>(elem) == "1");
        } else if(elem.name() == "bio") {
            var.wiki(jbson::get<wiki_t>(elem));
        } else if(elem.name() == "similar") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "artist")
                        var.similar(jbson::get<std::vector<artist>>(e));
            } else {
                var.similar(jbson::get<std::vector<artist>>(elem));
            }
        } else if(elem.name() == "tags") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "tag")
                        var.tags(jbson::get<std::vector<tag>>(e));
            } else {
                var.tags(jbson::get<std::vector<tag>>(elem));
            }
        }
    }
}

} // namespace lastfm

#endif // LASTFM_ARTIST_HPP
