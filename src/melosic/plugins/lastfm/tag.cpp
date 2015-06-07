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

#include <boost/range/combine.hpp>

#include <boost/hana/ext/boost/tuple.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/ext/std/vector.hpp>

#include <jbson/path.hpp>

#include "artist.hpp"
#include "album.hpp"
#include "track.hpp"
#include "service.hpp"
#include "tag.hpp"

namespace lastfm {

std::string_view tag::name() const {
    return m_name;
}

void tag::name(std::string_view name) {
    m_name = name.to_string();
}

const network::uri& tag::url() const {
    return m_url;
}

void tag::url(network::uri url) {
    m_url = std::move(url);
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

const wiki& tag::wiki() const {
    return m_wiki;
}

void tag::wiki(struct wiki wiki) {
    m_wiki = wiki;
}

std::future<tag> tag::get_info(service& serv, std::string_view name) {
    auto transformer = [](auto&& doc) {
        if(auto elem = vector_to_optional(jbson::path_select(doc, "tag"))) {
            return jbson::get<tag>(*elem);
        }
        throw std::runtime_error("invalid response from tag.getinfo");
    };
    return serv.get("tag.getinfo", make_params(std::make_pair("tag", name)), use_future, transformer);
}

std::future<tag> tag::get_info(service& serv) const {
    return get_info(serv, m_name);
}

std::future<std::vector<tag>> tag::get_similar(service& serv, std::string_view name) {
    auto transformer = [](auto&& doc) {
        return transform_copy(jbson::path_select(doc, "similartags.tag.*"), deserialise<tag>);
    };
    return serv.get("tag.getsimilar", make_params(std::make_pair("tag", name)), use_future, transformer);
}

std::future<std::vector<tag>> tag::get_similar(service& serv) const {
    return get_similar(serv, m_name);
}

std::future<std::vector<album>> tag::get_top_albums(service& serv, std::string_view name, std::optional<int> limit,
                                                    std::optional<int> page) {
    auto transformer = [](auto&& doc) {
        return transform_copy(jbson::path_select(doc, "topalbums.album.*"), deserialise<album>);
    };
    return serv.get("tag.gettopalbums", make_params(std::make_tuple("tag", name), std::make_tuple("limit", limit),
                                                    std::make_tuple("page", page)),
                    use_future, transformer);
}

std::future<std::vector<album>> tag::get_top_albums(service& serv, std::optional<int> limit,
                                                    std::optional<int> page) const {
    return get_top_albums(serv, m_name, limit, page);
}

std::future<std::vector<artist>> tag::get_top_artists(service& serv, std::string_view name, std::optional<int> limit,
                                                      std::optional<int> page) {
    auto transformer = [](auto&& doc) {
        return transform_copy(jbson::path_select(doc, "topartists.artist.*"), deserialise<artist>);
    };
    return serv.get("tag.gettopartists", make_params(std::make_tuple("tag", name), std::make_tuple("limit", limit),
                                                     std::make_tuple("page", page)),
                    use_future, transformer);
}

std::future<std::vector<artist>> tag::get_top_artists(service& serv, std::optional<int> limit,
                                                      std::optional<int> page) const {
    return get_top_artists(serv, m_name, limit, page);
}

std::future<std::vector<tag>> tag::get_top_tags(service& serv) {
    auto transformer = [](auto&& doc) {
        return transform_copy(jbson::path_select(doc, "toptags.tag.*"), deserialise<tag>);
    };
    return serv.get("tag.gettoptags", {}, use_future, transformer);
}

std::future<std::vector<track>> tag::get_top_tracks(service& serv, std::string_view name, std::optional<int> limit,
                                                    std::optional<int> page) {
}

std::future<std::vector<track>> tag::get_top_tracks(service& serv, std::optional<int> limit,
                                                    std::optional<int> page) const {
    return get_top_tracks(serv, m_name, limit, page);
}

std::future<std::vector<artist>> tag::get_weekly_artist_chart(service& serv, std::string_view name,
                                                              std::optional<std::tuple<date_t, date_t>> date_range,
                                                              std::optional<int> limit) {
    std::optional<date_t> from, to;
    auto lift_tuple = [](auto&& opt_tuple) {
        return hana::transform(std::forward<decltype(opt_tuple)>(opt_tuple), hana::lift<hana::ext::std::Optional>);
    };
    auto tie_date_range = [&](auto&& tuple_opt) { std::tie(from, to) = std::forward<decltype(tuple_opt)>(tuple_opt); };
    hana::transform(date_range, hana::compose(tie_date_range, lift_tuple));

    auto transformer = [](auto&& doc) {
        return transform_copy(jbson::path_select(doc, "weeklyartistchart.artist.*"), deserialise<artist>);
    };
    return serv.get("tag.getweeklyartistchart", make_params(std::make_tuple("tag", name), std::make_tuple("from", from),
                                                            std::make_tuple("to", to), std::make_tuple("limit", limit)),
                    use_future, transformer);
}

std::future<std::vector<artist>> tag::get_weekly_artist_chart(service& serv,
                                                              std::optional<std::tuple<date_t, date_t>> date_range,
                                                              std::optional<int> limit) const {
    return get_weekly_artist_chart(serv, m_name, date_range, limit);
}

std::future<std::vector<std::tuple<date_t, date_t>>> tag::get_weekly_chart_list(service& serv, std::string_view name) {
    auto transformer = [](auto&& doc) {
        auto from = jbson::path_select(doc, "weeklychartlist.chart.*.from");
        auto to = jbson::path_select(doc, "weeklychartlist.chart.*.to");

        std::vector<std::tuple<date_t, date_t>> charts{};

        boost::transform(from, to, std::back_inserter(charts), [](auto&& from, auto&& to) {
            return std::make_tuple(deserialise<date_t>(from), deserialise<date_t>(to));
        });

        return charts;
    };
    return serv.get("tag.getweeklychartlist", make_params(std::make_tuple("tag", name)), use_future, transformer);
}

std::future<std::vector<std::tuple<date_t, date_t>>> tag::get_weekly_chart_list(service& serv) const {
    return get_weekly_chart_list(serv, m_name);
}

std::future<std::vector<tag>> tag::search(service& serv, std::string_view name, std::optional<int> limit,
                                          std::optional<int> page) {
    auto transformer = [](auto&& doc) {
        return transform_copy(jbson::path_select(doc, "results.tagmatches.tag.*"), deserialise<tag>);
    };
    return serv.get("tag.search", make_params(std::make_tuple("tag", name), std::make_tuple("limit", limit),
                                              std::make_tuple("page", page)),
                    use_future, transformer);
}

std::future<std::vector<tag>> tag::search(service& serv, std::optional<int> limit, std::optional<int> page) const {
    return search(serv, m_name, limit, page);
}

} // namespace lastfm
