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

#include "artist.hpp"
#include "service.hpp"
#include "tag.hpp"

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

void artist::url(const network::uri& url) {
    m_url = url;
}

const std::vector<artist>& artist::similar() const {
    return m_similar;
}

void artist::similar(std::vector<artist> similar) {
    m_similar = std::move(similar);
}

const std::vector<tag>& artist::tags() const {
    return m_tags;
}

void artist::tags(std::vector<tag> tags) {
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

const wiki_t& artist::wiki() const {
    return m_wiki;
}

void artist::wiki(wiki_t wiki) {
    m_wiki = wiki;
}

} // namespace lastfm
