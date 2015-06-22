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

#ifndef LASTFM_VENUE_HPP
#define LASTFM_VENUE_HPP

#include <jbson/element.hpp>

#include <lastfm/lastfm.hpp>

namespace lastfm {

struct LASTFM_EXPORT venue {
    explicit venue() = default;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& user_elem, venue& var) {
//    auto doc = jbson::get<jbson::element_type::document_element>(user_elem);
//    for(auto&& elem : doc) {
//    }
}

} // namespace lastfm

#endif // LASTFM_VENUE_HPP
