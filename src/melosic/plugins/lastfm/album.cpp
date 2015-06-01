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

#include "album.hpp"
#include "service.hpp"
#include "tag.hpp"

namespace lastfm {

std::string_view album::name() const {
    return m_name;
}

void album::name(std::string_view name) {
    m_name = name.to_string();
}

const artist& album::artist() const {
    return m_artist;
}

void album::artist(struct artist artist) {
    m_artist = artist;
}

const network::uri& album::url() const {
    return m_url;
}

void album::url(const network::uri& url) {
    m_url = url;
}

date_t album::release_date() const {
    return m_release_date;
}

void album::release_date(date_t release_date) {
    m_release_date = release_date;
}

const std::vector<tag>& album::tags() const {
    return m_tags;
}

void album::tags(std::vector<tag> tags) {
    m_tags = std::move(tags);
}

int album::listeners() const {
    return m_listeners;
}

void album::listeners(int listeners) {
    m_listeners = listeners;
}

int album::plays() const {
    return m_plays;
}

void album::plays(int plays) {
    m_plays = plays;
}

bool album::streamable() const {
    return m_streamable;
}

void album::streamable(bool streamable) {
    m_streamable = streamable;
}

const wiki_t& album::wiki() const {
    return m_wiki;
}

void album::wiki(wiki_t wiki) {
    m_wiki = wiki;
}

} // namespace lastfm
