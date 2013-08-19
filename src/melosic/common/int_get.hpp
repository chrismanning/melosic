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

#ifndef MELOSIC_INT_GET_HPP
#define MELOSIC_INT_GET_HPP

#include <cstdint>

#include <boost/variant.hpp>
#include <boost/mpl/index_of.hpp>
#include <boost/throw_exception.hpp>

#include <melosic/common/configvar.hpp>

namespace Melosic {
namespace Config {

namespace {

template <typename T>
using TypeIndex = typename boost::mpl::index_of<VarType::types, T>::type;

template <typename U>
const U& get_impl(const VarType& operand) {
    switch(operand.which()) {
        case TypeIndex<int32_t>::value:
            return static_cast<const U&>(boost::get<int32_t>(operand));
        case TypeIndex<uint32_t>::value:
            return static_cast<const U&>(boost::get<uint32_t>(operand));
        case TypeIndex<int64_t>::value:
            return static_cast<const U&>(boost::get<int64_t>(operand));
        case TypeIndex<uint64_t>::value:
            return static_cast<const U&>(boost::get<uint64_t>(operand));
        default:
            BOOST_THROW_EXCEPTION(boost::bad_get());
    }
}

}

template<typename U, class = typename std::enable_if<std::is_integral<U>::value && !std::is_same<U, bool>::value>::type>
U get(VarType& operand) {
    return get_impl<U>(const_cast<const VarType&>(operand));
}

template<typename U, class = typename std::enable_if<std::is_integral<U>::value && !std::is_same<U, bool>::value>::type>
const U& get(const VarType& operand) {
    return get_impl<U>(operand);
}


}
}

#endif // MELOSIC_INT_GET_HPP
