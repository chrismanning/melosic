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

#include <jbson/path.hpp>
#include <jbson/json_reader.hpp>

#include "artist.hpp"
#include "album.hpp"
#include "track.hpp"
#include "service.hpp"
#include "tag.hpp"
#include "error.hpp"

namespace lastfm {

tag::tag(std::string_view tag_name, const network::uri& url, int reach, int taggings, bool streamable, wiki_t wiki)
    : m_name(tag_name), m_url(url), m_reach(reach), m_taggings(taggings), m_streamable(streamable),
      m_wiki(std::move(wiki)) {
}

std::string_view tag::name() const {
    return m_name;
}

void tag::name(std::string_view name) {
    m_name = name.to_string();
}

const network::uri& tag::url() const {
    return m_url;
}

void tag::url(const network::uri& url) {
    m_url = url;
}

int tag::reach() const {
    return m_reach;
}

void tag::reach(int reach) {
    m_reach = reach;
}

int tag::taggings() const {
    return m_taggings;
}

void tag::taggings(int taggings) {
    m_taggings = taggings;
}

bool tag::streamable() const {
    return m_streamable;
}

void tag::streamable(bool streamable) {
    m_streamable = streamable;
}

wiki_t tag::wiki() const {
    return m_wiki;
}

void tag::wiki(wiki_t wiki) {
    m_wiki = wiki;
}

std::future<std::vector<tag>> tag::get_similar(service& serv) const {
    return serv.get("tag.getsimilar", {{"tag", m_name}}, use_future, [](auto&& response) {
        auto doc = jbson::read_json(response.body());
        check_error(doc);

        std::vector<tag> tags{};

        for(auto&& elem : jbson::path_select(doc, "similartags.tag.*")) {
            tags.push_back(jbson::get<tag>(elem));
        }

        return tags;
    });
}

boost::future<std::vector<album>> tag::get_top_albums(service&, int limit, int page) const {
    return boost::make_ready_future<std::vector<album>>();
}

boost::future<std::vector<artist>> tag::get_top_artists(service&, int limit, int page) const {
    return boost::make_ready_future<std::vector<artist>>();
}

boost::future<std::vector<tag>> tag::get_top_tags(service&) const {
    return boost::make_ready_future<std::vector<tag>>();
}

boost::future<std::vector<track>> tag::get_top_tracks(service&, int limit, int page) const {
    return boost::make_ready_future<std::vector<track>>();
}

boost::future<std::vector<artist>> tag::get_weekly_artist_chart(service&, date_t from, date_t to, int limit) const {
    return boost::make_ready_future<std::vector<artist>>();
}

boost::future<std::vector<std::tuple<date_t, date_t>>> tag::get_weekly_chart_list(service&) const {
    return boost::make_ready_future<std::vector<std::tuple<date_t, date_t>>>();
}

boost::future<std::vector<tag>> tag::search(service&, int limit, int page) const {
    return boost::make_ready_future<std::vector<tag>>();
}

} // namespace lastfm
