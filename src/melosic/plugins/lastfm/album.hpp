/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#ifndef LASTFM_ALBUM_HPP
#define LASTFM_ALBUM_HPP

#include <string>
#include <vector>

#include <network/uri.hpp>

#include "artist.hpp"
#include "tag.hpp"
#include "track.hpp"

namespace lastfm {

struct LASTFM_EXPORT album {
    explicit album() = default;

    std::string_view name() const;
    void name(std::string_view);

    const artist& artist() const;
    void artist(struct artist);

    const network::uri& url() const;
    void url(const network::uri&);

    date_t release_date() const;
    void release_date(date_t);

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

    std::future<std::vector<album>> get_similar(service&) const;

  private:
    std::string m_name;
    struct artist m_artist;
    network::uri m_url;
    date_t m_release_date;
    std::vector<tag> m_tags;
    std::vector<track> m_tracks;
    int m_listeners = 0;
    int m_plays = 0;
    bool m_streamable = false;
    wiki_t m_wiki;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& album_elem, album& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(album_elem);
    for(auto&& elem : doc) {
        if(elem.name() == "name") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.name({str.data(), str.size()});
        }
        else if(elem.name() == "artist") {
            var.artist(jbson::get<artist>(elem));
        } else if(elem.name() == "url") {
            var.url(jbson::get<network::uri>(elem));
        } else if(elem.name() == "listeners") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.listeners(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "plays") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.plays(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "wiki") {
            var.wiki(jbson::get<wiki_t>(elem));
        } else if(elem.name() == "releasedate") {
            var.release_date(jbson::get<date_t>(elem));
        } else if(elem.name() == "toptags") {
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

#endif // LASTFM_ALBUM_HPP
