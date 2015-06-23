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

#include <lastfm/service.hpp>
#include <lastfm/album.hpp>
#include <lastfm/track.hpp>
#include <lastfm/user.hpp>
#include <lastfm/artist.hpp>

namespace lastfm {

std::string_view artist::name() const {
    return m_name;
}

void artist::name(std::string_view name) {
    m_name = name.to_string();
}

const network::uri& artist::url() const {
    return m_url;
}

void artist::url(network::uri url) {
    m_url = std::move(url);
}

const std::vector<artist>& artist::similar() const {
    return m_similar;
}

void artist::similar(std::vector<artist> similar) {
    m_similar = std::move(similar);
}

const std::vector<tag>& artist::top_tags() const {
    return m_tags;
}

void artist::top_tags(std::vector<tag> tags) {
    m_tags = std::move(tags);
}

int artist::listeners() const {
    return m_listeners;
}

void artist::listeners(int listeners) {
    m_listeners = listeners;
}

int artist::plays() const {
    return m_plays;
}

void artist::plays(int plays) {
    m_plays = plays;
}

bool artist::streamable() const {
    return m_streamable;
}

void artist::streamable(bool streamable) {
    m_streamable = streamable;
}

const wiki& artist::wiki() const {
    return m_wiki;
}

void artist::wiki(struct wiki wiki) {
    m_wiki = wiki;
}

const std::vector<image>& artist::images() const {
    return m_images;
}

void artist::images(std::vector<image> images) {
    m_images = std::move(images);
}

boost::uuids::uuid artist::mbid() const {
    return m_mbid;
}

void artist::mbid(boost::uuids::uuid mbid) {
    m_mbid = mbid;
}

std::future<artist> artist::get_info(service& serv, boost::uuids::uuid mbid, std::optional<std::string_view> lang,
                                     bool autocorrect, std::optional<std::string_view> username) {
    return serv.get("artist.getinfo",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("lang", lang),
                                std::make_pair("autocorrect", autocorrect), std::make_pair("username", username)),
                    use_future, transform_select<artist>("artist"));
}

std::future<artist> artist::get_info(service& serv, std::string_view name, std::optional<std::string_view> lang,
                                     bool autocorrect, std::optional<std::string_view> username) {
    return serv.get("artist.getinfo",
                    make_params(std::make_pair("artist", name), std::make_pair("lang", lang),
                                std::make_pair("autocorrect", autocorrect), std::make_pair("username", username)),
                    use_future, transform_select<artist>("artist"));
}

std::future<artist> artist::get_info(service& serv, std::optional<std::string_view> lang, bool autocorrect,
                                     std::optional<std::string_view> username) const {
    return get_info(serv, m_name, lang, autocorrect, username);
}

std::future<artist> artist::get_correction(service& serv, std::string_view name) {
    return serv.get("artist.getcorrection", make_params(std::make_pair("artist", name)), use_future,
                    transform_select<artist>("corrections.correction.artist"));
}

std::future<artist> artist::get_correction(service& serv) const {
    return get_correction(serv, m_name);
}

std::future<std::vector<event>> artist::get_events(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                   bool festivalsonly,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.getevents",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("festivalsonly", festivalsonly),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<event>>("events.event.*"));
}

std::future<std::vector<event>> artist::get_events(service& serv, std::string_view name, bool autocorrect,
                                                   bool festivalsonly,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.getevents",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("festivalsonly", festivalsonly),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<event>>("events.event.*"));
}

std::future<std::vector<event>> artist::get_events(service& serv, bool autocorrect,
                                                   bool festivalsonly, std::optional<int> limit,
                                                   std::optional<int> page) const {
    if(!m_mbid.is_nil())
        return get_events(serv, m_mbid, autocorrect, festivalsonly, limit, page);
    return get_events(serv, m_name, autocorrect, festivalsonly, limit, page);
}

std::future<std::vector<event>> artist::get_past_events(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.getpastevents",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<event>>("events.event.*"));
}

std::future<std::vector<event>> artist::get_past_events(service& serv, std::string_view name, bool autocorrect,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.getpastevents",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<event>>("events.event.*"));
}

std::future<std::vector<event>> artist::get_past_events(service& serv, bool autocorrect, std::optional<int> limit,
                                                   std::optional<int> page) const {
    if(!m_mbid.is_nil())
        return get_past_events(serv, m_mbid, autocorrect, limit, page);
    return get_past_events(serv, m_name, autocorrect, limit, page);
}

std::future<std::vector<shout>> artist::get_shouts(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.getshouts",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<shout>>("shouts.shout.*"));
}

std::future<std::vector<shout>> artist::get_shouts(service& serv, std::string_view name, bool autocorrect,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.getshouts",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<shout>>("shouts.shout.*"));
}

std::future<std::vector<shout>> artist::get_shouts(service& serv, bool autocorrect, std::optional<int> limit,
                                                   std::optional<int> page) const {
    if(!m_mbid.is_nil())
        return get_shouts(serv, m_mbid, autocorrect, limit, page);
    return get_shouts(serv, m_name, autocorrect, limit, page);
}

std::future<std::vector<artist>> artist::get_similar(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                     std::optional<int> limit) {
    return serv.get("artist.getsimilar",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit)),
                    use_future, transform_select<std::vector<artist>>("similarartists.artist.*"));
}

std::future<std::vector<artist>> artist::get_similar(service& serv, std::string_view name, bool autocorrect,
                                                     std::optional<int> limit) {
    return serv.get("artist.getsimilar",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit)),
                    use_future, transform_select<std::vector<artist>>("similarartists.artist.*"));
}

std::future<std::vector<artist>> artist::get_similar(service& serv, bool autocorrect, std::optional<int> limit) const {
    if(!m_mbid.is_nil())
        return get_similar(serv, m_mbid, autocorrect, limit);
    return get_similar(serv, m_name, autocorrect, limit);
}

std::future<std::vector<tag>> artist::get_tags(service& serv, boost::uuids::uuid mbid, std::string_view username,
                                               bool autocorrect, std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettags", make_params(std::make_pair("mbid", mbid), std::make_pair("username", username),
                                                  std::make_pair("autocorrect", autocorrect),
                                                  std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<tag>>("tags.tag.*"));
}

std::future<std::vector<tag>> artist::get_tags(service& serv, std::string_view name, std::string_view username,
                                               bool autocorrect, std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettags", make_params(std::make_pair("artist", name), std::make_pair("username", username),
                                                  std::make_pair("autocorrect", autocorrect),
                                                  std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<tag>>("tags.tag.*"));
}

std::future<std::vector<tag>> artist::get_tags(service& serv, std::string_view username, bool autocorrect,
                                               std::optional<int> limit, std::optional<int> page) const {
    return get_tags(serv, m_name, username, autocorrect, limit, page);
}

std::future<std::vector<album>> artist::get_top_albums(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                       std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettopalbums",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<album>>("topalbums.album.*"));
}

std::future<std::vector<album>> artist::get_top_albums(service& serv, std::string_view name, bool autocorrect,
                                                       std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettopalbums",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<album>>("topalbums.album.*"));
}

std::future<std::vector<album>> artist::get_top_albums(service& serv, bool autocorrect, std::optional<int> limit,
                                                       std::optional<int> page) const {
    if(!m_mbid.is_nil())
        return get_top_albums(serv, m_mbid, autocorrect, limit, page);
    return get_top_albums(serv, m_name, autocorrect, limit, page);
}

std::future<std::vector<user>> artist::get_top_fans(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                    std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettopfans",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<user>>("topfans.user.*"));
}

std::future<std::vector<user>> artist::get_top_fans(service& serv, std::string_view name, bool autocorrect,
                                                    std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettopfans",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<user>>("topfans.user.*"));
}

std::future<std::vector<user>> artist::get_top_fans(service& serv, bool autocorrect, std::optional<int> limit,
                                                    std::optional<int> page) const {
    if(!m_mbid.is_nil())
        return get_top_fans(serv, m_mbid, autocorrect, limit, page);
    return get_top_fans(serv, m_name, autocorrect, limit, page);
}

std::future<std::vector<tag>> artist::get_top_tags(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettoptags",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<tag>>("toptags.tag.*"));
}

std::future<std::vector<tag>> artist::get_top_tags(service& serv, std::string_view name, bool autocorrect,
                                                   std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettoptags",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<tag>>("toptags.tag.*"));
}

std::future<std::vector<tag>> artist::get_top_tags(service& serv, bool autocorrect, std::optional<int> limit,
                                                   std::optional<int> page) const {
    if(!m_mbid.is_nil())
        return get_top_tags(serv, m_mbid, autocorrect, limit, page);
    return get_top_tags(serv, m_name, autocorrect, limit, page);
}

std::future<std::vector<track>> artist::get_top_tracks(service& serv, boost::uuids::uuid mbid, bool autocorrect,
                                                       std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettoptracks",
                    make_params(std::make_pair("mbid", mbid), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<track>>("toptracks.track.*"));
}

std::future<std::vector<track>> artist::get_top_tracks(service& serv, std::string_view name, bool autocorrect,
                                                       std::optional<int> limit, std::optional<int> page) {
    return serv.get("artist.gettoptracks",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit), std::make_pair("page", page)),
                    use_future, transform_select<std::vector<track>>("toptracks.track.*"));
}

std::future<std::vector<track>> artist::get_top_tracks(service& serv, bool autocorrect, std::optional<int> limit,
                                                       std::optional<int> page) const {
    if(!m_mbid.is_nil())
        return get_top_tracks(serv, m_mbid, autocorrect, limit, page);
    return get_top_tracks(serv, m_name, autocorrect, limit, page);
}

std::future<std::vector<artist>> artist::search(service& serv, std::string_view name, std::optional<int> limit,
                                                std::optional<int> page) {
    return serv.get("artist.search", make_params(std::make_pair("artist", name), std::make_pair("limit", limit),
                                                 std::make_pair("page", page)),
                    use_future, transform_select<std::vector<artist>>("results.artistmatches.artist.*"));
}

std::future<std::vector<artist>> artist::search(service& serv, std::optional<int> limit,
                                                std::optional<int> page) const {
    return search(serv, m_name, limit, page);
}

} // namespace lastfm
