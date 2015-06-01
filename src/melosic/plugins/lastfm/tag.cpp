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

#include <boost/hana.hpp>
namespace hana = boost::hana;

#include "artist.hpp"
#include "album.hpp"
#include "track.hpp"
#include "service.hpp"
#include "tag.hpp"
#include "error.hpp"

namespace boost::hana {

    namespace ext::std {
        struct Optional;
    }

    template <typename T> struct datatype<::std::optional<T>> { using type = ext::std::Optional; };

    template <> struct lift_impl<ext::std::Optional> {
        template <typename X> static constexpr decltype(auto) apply(X&& x) {
            return std::make_optional(std::forward<X>(x));
        }
    };

    template <> struct ap_impl<ext::std::Optional> {
        template <typename F, typename... X>
        static constexpr auto apply_impl(std::false_type, F&& f, X&&... x)
            -> decltype(std::make_optional((*f)(x.value()...))) {
            if(f && (... && x))
                return std::make_optional((*f)(x.value()...));
            return std::nullopt;
        }
        template <typename F, typename... X> static constexpr void apply_impl(std::true_type, F&& f, X&&... x) {
            if(f && (... && x))
                (*f)(x.value()...);
        }
        template <typename F, typename... X> static constexpr decltype(auto) apply(F&& f, X&&... x) {
            return apply_impl(std::is_void<decltype((*f)(x.value()...))>{}, std::forward<F>(f), std::forward<X>(x)...);
        }
    };

    template <> struct transform_impl<ext::std::Optional> : Applicative::transform_impl<ext::std::Optional> {};
}

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

auto to_string = hana::overload(
    [](date_t t) {
        auto time = date_t::clock::to_time_t(t);
        std::stringstream ss{};
        ss << std::put_time(std::gmtime(&time), "%a, %d %b %Y %H:%M:%S");
        return ss.str();
    },
    [](std::string_view s) { return s.to_string(); }, [](auto&& i) { return ::std::to_string(i); });

template <typename StringT, typename OptT> auto make_param(std::tuple<StringT, std::optional<OptT>> t) {
    return std::make_tuple(std::string_view{get<0>(t)}, hana::transform(get<1>(t), to_string));
}

template <typename StringT, typename OptT> auto make_param(std::tuple<StringT, OptT> t) {
    return std::make_tuple(std::string_view{get<0>(t)}, std::make_optional(to_string(get<1>(t))));
}

template <template <typename...> typename TupleT, typename... StringT, typename... OptT>
params_t make_params(TupleT<StringT, OptT>&&... optional_params) {
    params_t params;
    for(auto&& param : {make_param(optional_params)...}) {
        hana::transform(get<1>(param),
                        [&, & name = get<0>(param) ](auto&& just) { return params.emplace_back(name, just); });
    }
    return params;
}
}

std::future<std::vector<tag>> tag::get_similar(service& serv, std::string_view name) {
    return serv.get("tag.getsimilar", make_params(std::make_tuple("tag", name)), use_future, [](auto&& response) {
        auto doc = jbson::read_json(response.body());
        check_error(doc);

        std::vector<tag> tags{};

        for(auto&& elem : jbson::path_select(doc, "similartags.tag.*")) {
            tags.push_back(jbson::get<tag>(elem));
        }

        return tags;
    });
}

std::future<std::vector<tag>> tag::get_similar(service& serv) const {
    return get_similar(serv, m_name);
}

std::future<std::vector<album>> tag::get_top_albums(service& serv, std::string_view name, std::optional<int> limit,
                                                    std::optional<int> page) {
    return serv.get("tag.gettopalbums", make_params(std::make_tuple("tag", name), std::make_tuple("limit", limit),
                                                    std::make_tuple("page", page)),
                    use_future, [](auto&& response) {
                        auto doc = jbson::read_json(response.body());
                        check_error(doc);

                        std::vector<album> albums{};

                        for(auto&& elem : jbson::path_select(doc, "topalbums.album.*")) {
                            albums.push_back(jbson::get<album>(elem));
                        }

                        return albums;
                    });
}

std::future<std::vector<album>> tag::get_top_albums(service& serv, std::optional<int> limit,
                                                    std::optional<int> page) const {
    return get_top_albums(serv, m_name, limit, page);
}

std::future<std::vector<artist>> tag::get_top_artists(service& serv, std::string_view name, std::optional<int> limit,
                                                      std::optional<int> page) {
    return serv.get("tag.gettopartists", make_params(std::make_tuple("tag", name), std::make_tuple("limit", limit),
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

std::future<std::vector<artist>> tag::get_top_artists(service& serv, std::optional<int> limit,
                                                      std::optional<int> page) const {
    return get_top_artists(serv, m_name, limit, page);
}

std::future<std::vector<tag>> tag::get_top_tags(service& serv) {
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

std::future<std::vector<track>> tag::get_top_tracks(service& serv, std::string_view name, std::optional<int> limit,
                                                    std::optional<int> page) {
}

std::future<std::vector<track>> tag::get_top_tracks(service& serv, std::optional<int> limit,
                                                    std::optional<int> page) const {
    return get_top_tracks(serv, m_name, limit, page);
}

std::future<std::vector<artist>> tag::get_weekly_artist_chart(service& serv, std::string_view name,
                                                              std::optional<date_t> from, std::optional<date_t> to,
                                                              std::optional<int> limit) {
}

std::future<std::vector<artist>> tag::get_weekly_artist_chart(service& serv, std::optional<date_t> from,
                                                              std::optional<date_t> to,
                                                              std::optional<int> limit) const {
    return get_weekly_artist_chart(serv, m_name, from, to, limit);
}

std::future<std::vector<std::tuple<date_t, date_t>>> tag::get_weekly_chart_list(service& serv, std::string_view name) {
}

std::future<std::vector<std::tuple<date_t, date_t>>> tag::get_weekly_chart_list(service& serv) const {
    return get_weekly_chart_list(serv, m_name);
}

std::future<std::vector<tag>> tag::search(service& serv, std::string_view name, std::optional<int> limit,
                                          std::optional<int> page) {
    return serv.get("tag.search", make_params(std::make_tuple("tag", name), std::make_tuple("limit", limit),
                                              std::make_tuple("page", page)),
                    use_future, [](auto&& response) {
                        auto doc = jbson::read_json(response.body());
                        check_error(doc);

                        std::vector<tag> tags{};

                        for(auto&& elem : jbson::path_select(doc, "results.tagmatches.tag.*")) {
                            tags.push_back(jbson::get<tag>(elem));
                        }

                        return tags;
                    });
}

std::future<std::vector<tag>> tag::search(service& serv, std::optional<int> limit, std::optional<int> page) const {
    return search(serv, m_name, limit, page);
}

} // namespace lastfm
