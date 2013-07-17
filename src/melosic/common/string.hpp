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

#ifndef MELOSIC_STRING_HPP
#define MELOSIC_STRING_HPP

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <list>
#include <string>

namespace Melosic {

#define toUpper(str) boost::algorithm::to_upper_copy(str)
#define toLower(str) boost::algorithm::to_lower_copy(str)

inline std::string toTitle(const std::string& str) {
    std::list<std::string> list;
    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char> > tokens(str, sep);
    for(const auto& s : tokens) {
        std::string tmp(toLower(s));
        tmp[0] = toupper(tmp[0]);
        list.push_back(tmp);
    }
    return boost::algorithm::join(list, " ");
}

inline namespace Literals {
    inline std::basic_string<char> operator "" _str(char const* str, size_t len) {
        return std::basic_string<char>(str, len);
    }

    inline std::basic_string<wchar_t> operator "" _str(wchar_t const* str, size_t len) {
        return std::basic_string<wchar_t>(str, len);
    }

    inline std::basic_string<char16_t> operator "" _str(char16_t const* str, size_t len) {
        return std::basic_string<char16_t>(str, len);
    }

    inline std::basic_string<char32_t> operator "" _str(char32_t const* str, size_t len) {
        return std::basic_string<char32_t>(str, len);
    }
}

}

#endif // MELOSIC_STRING_HPP
