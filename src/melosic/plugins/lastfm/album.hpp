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

#include <boost/uuid/uuid.hpp>

#include <lastfm/lastfm.hpp>
#include <lastfm/artist.hpp>
#include <lastfm/shout.hpp>
#include <lastfm/affiliation.hpp>
#include <lastfm/image.hpp>
#include <lastfm/tag.hpp>
#include <lastfm/mbid.hpp>

namespace lastfm {

struct track;

struct LASTFM_EXPORT album {
    explicit album() = default;

    std::string_view name() const;
    void name(std::string_view);

    const artist& artist() const;
    void artist(struct artist);

    const network::uri& url() const;
    void url(network::uri);

    date_t release_date() const;
    void release_date(date_t);

    const std::vector<track>& tracks() const;
    void tracks(std::vector<track>);

    const std::vector<tag>& top_tags() const;
    void top_tags(std::vector<tag>);

    int listeners() const;
    void listeners(int);

    int plays() const;
    void plays(int);

    bool streamable() const;
    void streamable(bool);

    const wiki& wiki() const;
    void wiki(struct wiki);

    const std::vector<shout>& shouts() const;
    void shouts(std::vector<shout>);

    const std::vector<image>& images() const;
    void images(std::vector<image>);

    mbid_t mbid() const;
    void mbid(mbid_t);

    // api methods

    static pplx::task<album> get_info(service&, mbid_t mbid,
                                      std::optional<std::string_view> lang = std::nullopt, bool autocorrect = false,
                                      std::optional<std::string_view> username = std::nullopt);
    static pplx::task<album> get_info(service&, std::string_view name, std::string_view artist,
                                      std::optional<std::string_view> lang = std::nullopt, bool autocorrect = false,
                                      std::optional<std::string_view> username = std::nullopt);
    pplx::task<album> get_info(service&, std::optional<std::string_view> lang = std::nullopt, bool autocorrect = false,
                               std::optional<std::string_view> username = std::nullopt) const;

    static pplx::task<std::vector<affiliation>> get_buy_links(service&, mbid_t mbid,
                                                              std::string_view countrycode, bool autocorrect = false);
    static pplx::task<std::vector<affiliation>> get_buy_links(service&, std::string_view name, std::string_view artist,
                                                              std::string_view countrycode, bool autocorrect = false);
    pplx::task<std::vector<affiliation>> get_buy_links(service&, std::string_view countrycode,
                                                       bool autocorrect = false) const;

    static pplx::task<std::vector<shout>> get_shouts(service&, mbid_t mbid, bool autocorrect = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<shout>> get_shouts(service&, std::string_view name, std::string_view artist,
                                                     bool autocorrect = false, std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    pplx::task<std::vector<shout>> get_shouts(service&, bool autocorrect = false,
                                              std::optional<int> limit = std::nullopt,
                                              std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<tag>> get_top_tags(service&, mbid_t mbid, bool autocorrect = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<tag>> get_top_tags(service&, std::string_view name, std::string_view artist,
                                                     bool autocorrect = false, std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    pplx::task<std::vector<tag>> get_top_tags(service&, bool autocorrect = false,
                                              std::optional<int> limit = std::nullopt,
                                              std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<tag>> get_tags(service&, mbid_t mbid, std::string_view username,
                                                 bool autocorrect = false, std::optional<int> limit = std::nullopt,
                                                 std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<tag>> get_tags(service&, std::string_view name, std::string_view artist,
                                                 std::string_view username, bool autocorrect = false,
                                                 std::optional<int> limit = std::nullopt,
                                                 std::optional<int> page = std::nullopt);
    pplx::task<std::vector<tag>> get_tags(service&, std::string_view username, bool autocorrect = false,
                                          std::optional<int> limit = std::nullopt,
                                          std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<album>> search(service&, std::string_view name,
                                                 std::optional<int> limit = std::nullopt,
                                                 std::optional<int> page = std::nullopt);
    pplx::task<std::vector<album>> search(service&, std::optional<int> limit = std::nullopt,
                                          std::optional<int> page = std::nullopt) const;

  private:
    std::string m_name;
    struct artist m_artist;
    network::uri m_url;
    date_t m_release_date;
    std::vector<tag> m_top_tags;
    std::vector<track> m_tracks;
    int m_listeners = 0;
    int m_plays = 0;
    bool m_streamable = false;
    struct wiki m_wiki;
    std::vector<shout> m_shouts;
    std::vector<image> m_images;
    mbid_t m_mbid{{0}};
};

template <typename Container> void value_get(const jbson::basic_element<Container>& album_elem, album& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(album_elem);
    for(auto&& elem : doc) {
        if(elem.name() == "name" || elem.name() == "title") {
            var.name(jbson::get<jbson::element_type::string_element>(elem));
        } else if(elem.name() == "artist") {
            if(elem.type() == jbson::element_type::string_element) {
                artist a;
                a.name(jbson::get<jbson::element_type::string_element>(elem));
                var.artist(std::move(a));
            } else
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
            var.wiki(jbson::get<wiki>(elem));
        } else if(elem.name() == "releasedate") {
            var.release_date(jbson::get<date_t>(elem));
        } else if(elem.name() == "toptags" || elem.name() == "tags") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "tag")
                        var.top_tags(jbson::get<std::vector<tag>>(e));
            } else {
                var.top_tags(jbson::get<std::vector<tag>>(elem));
            }
        } else if(elem.name() == "tracks") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "track")
                        var.tracks(jbson::get<std::vector<track>>(e));
            } else {
                var.tracks(jbson::get<std::vector<track>>(elem));
            }
        } else if(elem.name() == "image") {
            var.images(jbson::get<std::vector<image>>(elem));
        } else if(elem.name() == "mbid") {
            var.mbid(jbson::get<mbid_t>(elem));
        }
    }
}

} // namespace lastfm

#endif // LASTFM_ALBUM_HPP
