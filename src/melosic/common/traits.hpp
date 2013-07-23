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

//some useful type traits

#include <type_traits>

namespace Melosic {

template <template <class> class Trait, typename ...Args>
struct MultiArgsTrait : std::true_type {};

template <template <class> class Trait, typename T1, typename ...Args>
struct MultiArgsTrait<Trait, T1, Args...> {
    static constexpr bool value = Trait<T1>::value && MultiArgsTrait<Trait, Args...>::value;
    typedef MultiArgsTrait<Trait, T1, Args...> type;
};

}

#endif // MELOSIC_TRAITS_HPP
