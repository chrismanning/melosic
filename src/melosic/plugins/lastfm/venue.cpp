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

#include <lastfm/venue.hpp>

namespace lastfm {

std::string_view venue::id() const {
    return m_id;
}

void venue::id(std::string_view id) {
    m_id = id.to_string();
}

std::string_view venue::name() const {
    return m_name;
}

void venue::name(std::string_view name) {
    m_name = name.to_string();
}

const network::uri& venue::url() const {
    return m_url;
}

void venue::url(network::uri url) {
    m_url = std::move(url);
}

const network::uri& venue::website() const {
    return m_website;
}

void venue::website(network::uri website) {
    m_website = std::move(website);
}

const std::vector<image>& venue::images() const {
    return m_images;
}

void venue::images(std::vector<image> images) {
    m_images = std::move(images);
}

} // namespace lastfm
