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

#include <lastfm/track.hpp>
#include <lastfm/tag.hpp>
#include <lastfm/service.hpp>

namespace lastfm {

std::string_view track::name() const {
    return m_name;
}

void track::name(std::string_view name) {
    m_name = name.to_string();
}

const artist& track::artist() const {
    return m_artist;
}

void track::artist(struct artist artist) {
    m_artist = std::move(artist);
}

const album& track::album() const {
    return m_album;
}

void track::album(struct album album) {
    m_album = std::move(album);
}

int track::tracknumber() const {
    return m_tracknumber;
}

void track::tracknumber(int tracknumber) {
    m_tracknumber = tracknumber;
}

std::chrono::milliseconds track::duration() const {
    return m_duration;
}

void track::duration(std::chrono::milliseconds duration) {
    m_duration = duration;
}

const std::vector<tag>& track::top_tags() const {
    return m_tags;
}

void track::top_tags(std::vector<tag> tags) {
    m_tags = std::move(tags);
}

const std::vector<track>& track::similar() const {
    return m_similar;
}

void track::similar(std::vector<track> similar) {
    m_similar = std::move(similar);
}

const network::uri& track::url() const {
    return m_url;
}

void track::url(network::uri url) {
    m_url = std::move(url);
}

int track::listeners() const {
    return m_listeners;
}

void track::listeners(int listeners) {
    m_listeners = listeners;
}

int track::plays() const {
    return m_plays;
}

void track::plays(int plays) {
    m_plays = plays;
}

bool track::streamable() const {
    return m_streamable;
}

void track::streamable(bool streamable) {
    m_streamable = streamable;
}

const wiki& track::wiki() const {
    return m_wiki;
}

void track::wiki(struct wiki wiki) {
    m_wiki = std::move(wiki);
}

pplx::task<track> track::get_info(service& serv, std::string_view name, std::string_view artist, bool autocorrect,
                                  std::optional<std::string_view> username) {
    return serv.get("track.getinfo",
                    detail::make_params(std::make_pair("track", name), std::make_pair("artist", artist),
                                        std::make_pair("autocorrect", autocorrect),
                                        std::make_pair("username", username)),
                    transform_select<track>("track"));
}

pplx::task<track> track::get_info(service& serv, bool autocorrect, std::optional<std::string_view> username) const {
    return get_info(serv, m_name, m_artist.name(), autocorrect, username);
}

} // namespace lastfm
