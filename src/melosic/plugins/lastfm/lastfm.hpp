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

#ifndef LASTFM_HPP
#define LASTFM_HPP

#include <chrono>
#include <experimental/string_view>
#include <experimental/optional>

#include <pplx/pplxtasks.h>

#include <network/uri.hpp>

#include <jbson/element.hpp>

#include <lastfm/exports.hpp>

namespace lastfm {

constexpr std::string_view operator""_sv(const char* str, size_t len) {
    return {str, len};
}

constexpr std::wstring_view operator""_sv(const wchar_t* str, size_t len) {
    return {str, len};
}

constexpr std::u16string_view operator""_sv(const char16_t* str, size_t len) {
    return {str, len};
}

constexpr std::u32string_view operator""_sv(const char32_t* str, size_t len) {
    return {str, len};
}

using date_t = std::chrono::system_clock::time_point;

} // namespace lastfm

namespace std {
namespace experimental {}
using namespace experimental;
namespace chrono {

template <typename Container> void value_get(const jbson::basic_element<Container>& elem, lastfm::date_t& var) {
    std::tm tm;
    auto str = jbson::get<std::string>(elem);
    std::stringstream ss{str};
    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");
    var = lastfm::date_t::clock::from_time_t(std::mktime(&tm));
}

} // namespace chrono

template <typename Container, typename Elem>
void value_get(const jbson::basic_element<Container>& vector_elem, vector<Elem>& var) {
    auto arr = jbson::get<jbson::element_type::array_element>(vector_elem);
    for(auto&& elem : arr) {
        var.push_back(jbson::get<Elem>(elem));
    }
}

} // namespace std

namespace network {

template <typename Container> void value_get(const jbson::basic_element<Container>& elem, uri& var) {
    auto str = jbson::get<jbson::element_type::string_element>(elem);
    var = uri{std::begin(str), std::end(str)};
}

} // namespace network

#endif // LASTFM_HPP
