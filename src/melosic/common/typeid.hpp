/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#ifndef MELOSIC_TYPEID_HPP
#define MELOSIC_TYPEID_HPP

#include <ostream>
#include <typeinfo>

namespace std {
inline std::ostream& operator<<(std::ostream& strm, const std::type_info& t) {
    const auto str = abi::__cxa_demangle(t.name(), nullptr, nullptr, nullptr);
    if(str) {
        strm << str;
        ::free(str);
    }
    return strm;
}
} //std

#endif // MELOSIC_TYPEID_HPP
