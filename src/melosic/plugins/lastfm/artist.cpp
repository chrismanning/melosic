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

#include <lastfm/artist.hpp>
#include <lastfm/tag.hpp>
#include <lastfm/service.hpp>

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

std::future<std::vector<artist>> artist::get_similar(service& serv, std::string_view name, bool autocorrect,
                                                     std::optional<int> limit) {
    return serv.get("artist.getsimilar",
                    make_params(std::make_pair("artist", name), std::make_pair("autocorrect", autocorrect),
                                std::make_pair("limit", limit)),
                    use_future, transform_select<std::vector<artist>>("similarartists.artist"));
}

std::future<std::vector<artist>> artist::get_similar(service& serv, bool autocorrect, std::optional<int> limit) const {
}

} // namespace lastfm
