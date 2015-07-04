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

#include <boost/uuid/uuid.hpp>

#include <lastfm/lastfm.hpp>
#include <lastfm/wiki.hpp>
#include <lastfm/image.hpp>
#include <lastfm/shout.hpp>
#include <lastfm/event.hpp>
#include <lastfm/tag.hpp>
#include <lastfm/mbid.hpp>

namespace lastfm {

class service;
struct album;
struct user;
struct track;

struct LASTFM_EXPORT artist {
    explicit artist() = default;

    std::string_view name() const;
    void name(std::string_view);

    const network::uri& url() const;
    void url(network::uri);

    const std::vector<artist>& similar() const;
    void similar(std::vector<artist>);

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

    const std::vector<image>& images() const;
    void images(std::vector<image>);

    mbid_t mbid() const;
    void mbid(mbid_t);

    // api methods

    static pplx::task<artist> get_info(service&, mbid_t mbid,
                                       std::optional<std::string_view> lang = std::nullopt, bool autocorrect = false,
                                       std::optional<std::string_view> username = std::nullopt);
    static pplx::task<artist> get_info(service&, std::string_view name,
                                       std::optional<std::string_view> lang = std::nullopt, bool autocorrect = false,
                                       std::optional<std::string_view> username = std::nullopt);
    pplx::task<artist> get_info(service&, std::optional<std::string_view> lang = std::nullopt, bool autocorrect = false,
                                std::optional<std::string_view> username = std::nullopt) const;

    static pplx::task<artist> get_correction(service&, std::string_view name);
    pplx::task<artist> get_correction(service&) const;

    static pplx::task<std::vector<event>> get_events(service&, mbid_t mbid, bool autocorrect = false,
                                                     bool festivalsonly = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<event>> get_events(service&, std::string_view name, bool autocorrect = false,
                                                     bool festivalsonly = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    pplx::task<std::vector<event>> get_events(service&, bool autocorrect = false, bool festivalsonly = false,
                                              std::optional<int> limit = std::nullopt,
                                              std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<event>> get_past_events(service&, mbid_t mbid, bool autocorrect = false,
                                                          std::optional<int> limit = std::nullopt,
                                                          std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<event>> get_past_events(service&, std::string_view name, bool autocorrect = false,
                                                          std::optional<int> limit = std::nullopt,
                                                          std::optional<int> page = std::nullopt);
    pplx::task<std::vector<event>> get_past_events(service&, bool autocorrect = false,
                                                   std::optional<int> limit = std::nullopt,
                                                   std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<shout>> get_shouts(service&, mbid_t mbid, bool autocorrect = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<shout>> get_shouts(service&, std::string_view name, bool autocorrect = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    pplx::task<std::vector<shout>> get_shouts(service&, bool autocorrect = false,
                                              std::optional<int> limit = std::nullopt,
                                              std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<artist>> get_similar(service&, mbid_t mbid, bool autocorrect = false,
                                                       std::optional<int> limit = std::nullopt);
    static pplx::task<std::vector<artist>> get_similar(service&, std::string_view name, bool autocorrect = false,
                                                       std::optional<int> limit = std::nullopt);
    pplx::task<std::vector<artist>> get_similar(service&, bool autocorrect = false,
                                                std::optional<int> limit = std::nullopt) const;

    static pplx::task<std::vector<tag>> get_tags(service&, mbid_t mbid, std::string_view username,
                                                 bool autocorrect = false, std::optional<int> limit = std::nullopt,
                                                 std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<tag>> get_tags(service&, std::string_view name, std::string_view username,
                                                 bool autocorrect = false, std::optional<int> limit = std::nullopt,
                                                 std::optional<int> page = std::nullopt);
    pplx::task<std::vector<tag>> get_tags(service&, std::string_view username, bool autocorrect = false,
                                          std::optional<int> limit = std::nullopt,
                                          std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<album>> get_top_albums(service&, mbid_t mbid, bool autocorrect = false,
                                                         std::optional<int> limit = std::nullopt,
                                                         std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<album>> get_top_albums(service&, std::string_view name, bool autocorrect = false,
                                                         std::optional<int> limit = std::nullopt,
                                                         std::optional<int> page = std::nullopt);
    pplx::task<std::vector<album>> get_top_albums(service&, bool autocorrect = false,
                                                  std::optional<int> limit = std::nullopt,
                                                  std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<user>> get_top_fans(service&, mbid_t mbid, bool autocorrect = false,
                                                      std::optional<int> limit = std::nullopt,
                                                      std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<user>> get_top_fans(service&, std::string_view name, bool autocorrect = false,
                                                      std::optional<int> limit = std::nullopt,
                                                      std::optional<int> page = std::nullopt);
    pplx::task<std::vector<user>> get_top_fans(service&, bool autocorrect = false,
                                               std::optional<int> limit = std::nullopt,
                                               std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<tag>> get_top_tags(service&, mbid_t mbid, bool autocorrect = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<tag>> get_top_tags(service&, std::string_view name, bool autocorrect = false,
                                                     std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt);
    pplx::task<std::vector<tag>> get_top_tags(service&, bool autocorrect = false,
                                              std::optional<int> limit = std::nullopt,
                                              std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<track>> get_top_tracks(service&, mbid_t mbid, bool autocorrect = false,
                                                         std::optional<int> limit = std::nullopt,
                                                         std::optional<int> page = std::nullopt);
    static pplx::task<std::vector<track>> get_top_tracks(service&, std::string_view name, bool autocorrect = false,
                                                         std::optional<int> limit = std::nullopt,
                                                         std::optional<int> page = std::nullopt);
    pplx::task<std::vector<track>> get_top_tracks(service&, bool autocorrect = false,
                                                  std::optional<int> limit = std::nullopt,
                                                  std::optional<int> page = std::nullopt) const;

    static pplx::task<std::vector<artist>> search(service&, std::string_view name,
                                                  std::optional<int> limit = std::nullopt,
                                                  std::optional<int> page = std::nullopt);
    pplx::task<std::vector<artist>> search(service&, std::optional<int> limit = std::nullopt,
                                           std::optional<int> page = std::nullopt) const;

  private:
    std::string m_name;
    network::uri m_url;
    std::vector<artist> m_similar;
    std::vector<tag> m_tags;
    int m_listeners = 0;
    int m_plays = 0;
    bool m_streamable = false;
    struct wiki m_wiki;
    std::vector<image> m_images;
    mbid_t m_mbid{{0}};
};

template <typename Container> void value_get(const jbson::basic_element<Container>& artist_elem, artist& var) {
    if(artist_elem.type() == jbson::element_type::string_element) {
        var.name(jbson::get<jbson::element_type::string_element>(artist_elem));
        return;
    }
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
            var.wiki(jbson::get<wiki>(elem));
        } else if(elem.name() == "similar") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "artist")
                        var.similar(jbson::get<std::vector<artist>>(e));
            } else {
                var.similar(jbson::get<std::vector<artist>>(elem));
            }
        } else if(elem.name() == "tags" || elem.name() == "toptags") {
            if(elem.type() == jbson::element_type::document_element) {
                for(auto&& e : jbson::get<jbson::element_type::document_element>(elem))
                    if(e.name() == "tag")
                        var.top_tags(jbson::get<std::vector<tag>>(e));
            } else {
                var.top_tags(jbson::get<std::vector<tag>>(elem));
            }
        } else if(elem.name() == "image") {
            var.images(jbson::get<std::vector<image>>(elem));
        } else if(elem.name() == "mbid") {
            var.mbid(jbson::get<mbid_t>(elem));
        }
    }
}

} // namespace lastfm

#endif // LASTFM_ARTIST_HPP
