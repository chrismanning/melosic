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

#include <lastfm/location.hpp>

std::string_view location::street() const {
    return m_street;
}

void location::street(std::string_view street) {
    m_street = street.to_string();
}

std::string_view location::city() const {
    return m_city;
}

void location::city(std::string_view city) {
    m_city = city.to_string();
}

std::string_view location::country() const {
    return m_country;
}

void location::country(std::string_view country) {
    m_country = country.to_string();
}

std::string_view location::post_code() const {
    return m_post_code;
}

void location::post_code(std::string_view post_code) {
    m_post_code = post_code.to_string();
}
