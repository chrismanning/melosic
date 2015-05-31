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

#ifndef LASTFM_TAG_HPP
#define LASTFM_TAG_HPP

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
#include "artist.hpp"

namespace lastfm {

class service;
struct album;
struct artist;
struct track;

struct LASTFM_EXPORT tag {
    explicit tag() = default;

    std::string_view name() const;
    void name(std::string_view);

    const network::uri& url() const;
    void url(const network::uri&);

    int reach() const;
    void reach(int);

    int taggings() const;
    void taggings(int);

    bool streamable() const;
    void streamable(bool);

    const wiki_t& wiki() const;
    void wiki(wiki_t);

    // api methods

    std::future<std::vector<tag>> get_similar(service&) const;
    std::future<std::vector<album>> get_top_albums(service&, std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt) const;
    std::future<std::vector<artist>> get_top_artists(service&, std::optional<int> limit = std::nullopt,
                                                     std::optional<int> page = std::nullopt) const;
    std::future<std::vector<tag>> get_top_tags(service&) const;
    boost::future<std::vector<track>> get_top_tracks(service&, int limit = 50, int page = 1) const;
    boost::future<std::vector<artist>> get_weekly_artist_chart(service&, date_t from = {}, date_t to = {},
                                                               int limit = 50) const;
    boost::future<std::vector<std::tuple<date_t, date_t>>> get_weekly_chart_list(service&) const;
    boost::future<std::vector<tag>> search(service&, int limit = 50, int page = 1) const;

  private:
    std::string m_name;
    network::uri m_url;
    int m_reach = 0;
    int m_taggings = 0;
    bool m_streamable = false;
    wiki_t m_wiki;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& tag_elem, tag& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(tag_elem);
    for(auto&& elem : doc) {
        if(elem.name() == "name") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.name({str.data(), str.size()});
        } else if(elem.name() == "url") {
            var.url(jbson::get<network::uri>(elem));
        } else if(elem.name() == "reach") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.reach(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "taggings") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.taggings(std::strtol(str.data(), nullptr, 10));
        } else if(elem.name() == "streamable") {
            var.streamable(jbson::get<jbson::element_type::string_element>(elem) == "1");
        } else if(elem.name() == "wiki") {
            var.wiki(jbson::get<wiki_t>(elem));
        }
    }
}

} // namespace lastfm

#endif // LASTFM_TAG_HPP
