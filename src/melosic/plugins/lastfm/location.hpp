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

#ifndef LASTFM_LOCATION_HPP
#define LASTFM_LOCATION_HPP

#include <lastfm/lastfm.hpp>

struct LASTFM_EXPORT location {
    explicit location() = default;

    std::string_view street() const;
    void street(std::string_view street);

    std::string_view city() const;
    void city(std::string_view city);

    std::string_view country() const;
    void country(std::string_view country);

    std::string_view post_code() const;
    void post_code(std::string_view post_code);

    // api methods

private:
    std::string m_street;
    std::string m_city;
    std::string m_country;
    std::string m_post_code;
};

using geo = location;

template <typename Container> void value_get(const jbson::basic_element<Container>& location_elem, location& var) {
    auto doc = jbson::get<jbson::element_type::document_element>(location_elem);
    for(auto&& elem : doc) {
        if(elem.name() == "street") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.street({str.data(), str.size()});
        } else if(elem.name() == "city") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.city({str.data(), str.size()});
        } else if(elem.name() == "postalcode") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.post_code({str.data(), str.size()});
        } else if(elem.name() == "country") {
            auto str = jbson::get<jbson::element_type::string_element>(elem);
            var.country({str.data(), str.size()});
        }
    }
}

#endif // LASTFM_LOCATION_HPP
