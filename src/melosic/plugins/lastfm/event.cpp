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

#include <lastfm/event.hpp>
#include <lastfm/artist.hpp>

namespace lastfm {

std::string_view event::id() const {
    return m_id;
}

void event::id(std::string_view id) {
    m_id = id.to_string();
}

std::string_view event::name() const {
    return m_name;
}

void event::name(std::string_view name) {
    m_name = name.to_string();
}

const std::vector<artist>& event::artists() const {
    return m_artists;
}

void event::artists(std::vector<artist> artists) {
    m_artists = std::move(artists);
}

const venue& event::venue() const {
    return m_venue;
}

void event::venue(struct venue venue) {
    m_venue = std::move(venue);
}

date_t event::start_date() const {
    return m_start_date;
}

void event::start_date(date_t start_date) {
    m_start_date = start_date;
}

std::string_view event::description() const {
    return m_description;
}

void event::description(std::string_view description) {
    m_description = description.to_string();
}

const std::vector<image>& event::images() const {
    return m_images;
}

void event::images(std::vector<image> images) {
    m_images = std::move(images);
}

int event::attendance() const {
    return m_attendance;
}

void event::attendance(int attendance) {
    m_attendance = attendance;
}

const network::uri& event::url() const {
    return m_url;
}

void event::url(network::uri url) {
    m_url = std::move(url);
}

} // namespace lastfm
