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

#ifndef LASTFM_TRACK_HPP
#define LASTFM_TRACK_HPP

#include <lastfm/album.hpp>
#include <lastfm/artist.hpp>
#include <lastfm/detail/transform.hpp>

namespace lastfm {

class service;
struct tag;

struct LASTFM_EXPORT track {
    explicit track() = default;

    std::string_view name() const;
    void name(std::string_view);

    const artist& artist() const;
    void artist(struct artist);

    const album& album() const;
    void album(struct album);

    int tracknumber() const;
    void tracknumber(int);

    std::chrono::milliseconds duration() const;
    void duration(std::chrono::milliseconds);

    const std::vector<tag>& top_tags() const;
    void top_tags(std::vector<tag>);

    const std::vector<track>& similar() const;
    void similar(std::vector<track>);

    const network::uri& url() const;
    void url(network::uri);

    int listeners() const;
    void listeners(int);

    int plays() const;
    void plays(int);

    bool streamable() const;
    void streamable(bool);

    const wiki& wiki() const;
    void wiki(struct wiki);

    // api methods

    static std::future<track> get_info(service&, std::string_view name, std::string_view artist,
                                       bool autocorrect = false,
                                       std::optional<std::string_view> username = std::nullopt);
    std::future<track> get_info(service&, bool autocorrect = false,
                                std::optional<std::string_view> username = std::nullopt) const;

  private:
    std::string m_name;
    struct artist m_artist;
    struct album m_album;
    int m_tracknumber = 0;
    std::chrono::milliseconds m_duration{0};
    std::vector<tag> m_tags;
    std::vector<track> m_similar;
    network::uri m_url;
    int m_listeners = 0;
    int m_plays = 0;
    bool m_streamable = false;
    struct wiki m_wiki;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& tag_elem, track& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(tag_elem);
    for(auto&& elem : doc) {
        if(elem.name() == "name") {
            var.name(jbson::get<jbson::element_type::string_element>(elem));
        } else if(elem.name() == "url") {
            var.url(jbson::get<network::uri>(elem));
        } else if(elem.name() == "artist") {
            var.artist(jbson::get<artist>(elem));
        } else if(elem.name() == "album") {
            var.album(jbson::get<album>(elem));
            if(auto num = vector_to_optional(jbson::path_select(jbson::get<jbson::document>(elem), "@attr.position"))) {
                auto str = jbson::get<jbson::element_type::string_element>(*num);
                var.tracknumber(std::strtol(str.data(), nullptr, 10));
            }
        } else if(elem.name() == "toptags" || elem.name() == "tags") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "tag")
                        var.top_tags(jbson::get<std::vector<tag>>(e));
            } else {
                var.top_tags(jbson::get<std::vector<tag>>(elem));
            }
        } else if(elem.name() == "similar") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "track")
                        var.similar(jbson::get<std::vector<track>>(e));
            } else {
                var.similar(jbson::get<std::vector<track>>(elem));
            }
        } else if(elem.name() == "duration") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.duration(std::chrono::milliseconds(std::strtol(str.data(), nullptr, 10)));
        } else if(elem.name() == "listeners") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.listeners(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "playcount") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.plays(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "streamable") {
            if(elem.type() == jbson::element_type::string_element)
                var.streamable(jbson::get<jbson::element_type::string_element>(elem) == "1");
            else if(elem.type() == jbson::element_type::document_element) {
                if(auto num = vector_to_optional(jbson::path_select(jbson::get<jbson::document>(elem), "#text")))
                    var.streamable(jbson::get<jbson::element_type::string_element>(*num) == "1");
            }
        } else if(elem.name() == "wiki") {
            var.wiki(jbson::get<wiki>(elem));
        }
    }
}

} // namespace lastfm

#endif // LASTFM_TRACK_HPP
