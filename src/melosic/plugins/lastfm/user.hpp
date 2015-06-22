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

#ifndef USER_HPP
#define USER_HPP

#include <network/uri.hpp>

#include <jbson/element.hpp>

#include <lastfm/lastfm.hpp>

namespace lastfm {
class service;

struct LASTFM_EXPORT user {
    explicit user() = default;
};

template <typename Container> void value_get(const jbson::basic_element<Container>& user_elem, user& var) {
//    auto doc = jbson::get<jbson::element_type::document_element>(user_elem);
//    for(auto&& elem : doc) {
//    }
}

} // namespace lastfm

#endif // USER_HPP
