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

#include <lastfm/shout.hpp>

namespace lastfm {

std::string_view shout::author() const {
    return m_author;
}

void shout::author(std::string_view author) {
    m_author = author.to_string();
}

std::string_view shout::body() const {
    return m_body;
}

void shout::body(std::string_view body) {
    m_body = body.to_string();
}

date_t shout::date() const {
    return m_date;
}

void shout::date(date_t date) {
    m_date = date;
}

} // namespace lastfm
