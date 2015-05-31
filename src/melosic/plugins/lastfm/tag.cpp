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

const wiki_t& tag::wiki() const {
    return m_wiki;
}

void tag::wiki(wiki_t wiki) {
    m_wiki = wiki;
}

namespace {

using std::get;

using params_t = service::params_t;

template <typename TupleT>
using optionalise_param_t = std::tuple<std::tuple_element_t<0, TupleT>, std::optional<std::tuple_element_t<1, TupleT>>>;

template <typename StringT, typename OptT> auto make_param(std::tuple<StringT, std::optional<OptT>> t) {
    return std::make_tuple(get<0>(t), get<1>(t) ? std::make_optional(std::to_string(*get<1>(t))) : std::nullopt);
}

template <typename StringT, typename OptT> auto make_param(std::tuple<StringT, OptT> t) {
    return std::make_tuple(get<0>(t), std::make_optional(get<1>(t)));
}

template <typename MaybeT, typename Fun>
auto apply(MaybeT&& maybe, Fun&& fun) -> std::enable_if_t<!std::is_void_v<decltype(fun(*maybe))>,
std::optional<decltype(fun(*maybe))>> {
    if(maybe)
        return fun(*maybe);
    return std::nullopt;
}

template <typename MaybeT, typename Fun>
auto apply(MaybeT&& maybe, Fun&& fun) -> void {
    if(maybe)
        fun(*maybe);
}

template <template <typename...> typename TupleT, typename... StringT, typename... OptT>
params_t make_params(TupleT<StringT, OptT>&&... optional_params) {
    params_t params;
    for(auto&& param : {make_param(optional_params)...}) {
        apply(get<1>(param), [&, &name=get<0>(param)](auto&& just) { return params.emplace_back(name, just); });
    }
    return params;
}

}

std::future<std::vector<tag>> tag::get_similar(service& serv) const {
    return serv.get("tag.getsimilar", make_params(std::make_tuple("tag", m_name)), use_future, [](auto&& response) {
        auto doc = jbson::read_json(response.body());
        check_error(doc);

        std::vector<tag> tags{};

        for(auto&& elem : jbson::path_select(doc, "similartags.tag.*")) {
            tags.push_back(jbson::get<tag>(elem));
        }

        return tags;
    });
}

std::future<std::vector<album>> tag::get_top_albums(service&, std::optional<int> limit, std::optional<int> page) const {
}

std::future<std::vector<artist>> tag::get_top_artists(service& serv, std::optional<int> limit,
                                                      std::optional<int> page) const {
    return serv.get("tag.gettopartists", make_params(std::make_tuple("tag", m_name), std::make_tuple("limit", limit),
                                                     std::make_tuple("page", page)),
                    use_future, [](auto&& response) {
                        auto doc = jbson::read_json(response.body());
                        check_error(doc);

                        std::vector<artist> artists{};

                        for(auto&& elem : jbson::path_select(doc, "topartists.artist.*")) {
                            artists.push_back(jbson::get<artist>(elem));
                        }

                        return artists;
                    });
}

std::future<std::vector<tag>> tag::get_top_tags(service& serv) const {
    return serv.get("tag.gettoptags", {}, use_future, [](auto&& response) {
        auto doc = jbson::read_json(response.body());
        check_error(doc);

        std::vector<tag> tags{};

        for(auto&& elem : jbson::path_select(doc, "toptags.tag.*")) {
            tags.push_back(jbson::get<tag>(elem));
        }

        return tags;
    });
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
