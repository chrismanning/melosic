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

#ifndef MELOSIC_RANGE_HPP
#define MELOSIC_RANGE_HPP

#include <type_traits>

#include <boost/range.hpp>
#include <boost/range/any_range.hpp>

namespace Melosic {
template <typename T>
using ForwardRange = boost::any_range<T,
                                      boost::forward_traversal_tag,
                                      typename std::add_lvalue_reference<T>::type,
                                      std::ptrdiff_t>;
template <typename T>
using BidirRange = boost::any_range<T,
                                    boost::bidirectional_traversal_tag,
                                    typename std::add_lvalue_reference<T>::type,
                                    std::ptrdiff_t>;
template <typename T>
using RandomRange = boost::any_range<T,
                                     boost::random_access_traversal_tag,
                                     typename std::add_lvalue_reference<T>::type,
                                     std::ptrdiff_t>;
}

#endif // MELOSIC_RANGE_HPP
