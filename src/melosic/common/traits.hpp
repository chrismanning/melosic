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

#ifndef MELOSIC_TRAITS_HPP
#define MELOSIC_TRAITS_HPP

#include <boost/mpl/apply.hpp>
#include <boost/mpl/logical.hpp>
namespace { namespace mpl = boost::mpl; }

#include <type_traits>

//some useful type traits

namespace Melosic {

template <class Trait, typename ...Args>
struct MultiArgsTrait : mpl::and_<mpl::true_, mpl::apply<Trait, Args>...>::type {};

template <class T>
struct ObjFromMemFunPtr {};

template <class T, class U>
struct ObjFromMemFunPtr<T U::*> {
    typedef typename std::decay<U>::type type;
};

}

#endif // MELOSIC_TRAITS_HPP
