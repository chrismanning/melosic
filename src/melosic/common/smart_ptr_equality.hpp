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

#ifndef SMART_PTR_EQUALITY_HPP
#define SMART_PTR_EQUALITY_HPP

#include <type_traits>
#include <memory>

template <template <typename...> class Ptr, typename T2, typename... PtrArgs>
bool operator==(Ptr<PtrArgs...>& ptr, T2* raw) {
    static_assert(sizeof...(PtrArgs) >= 1, "template Ptr should have at least 1 template parameter");
    static_assert(std::is_pointer<decltype(ptr.get())>::value, "Ptr must be smart pointer");
    typedef typename std::tuple_element<0, std::tuple<PtrArgs...>>::type T;
    static_assert(std::is_base_of<T, T2>::value || std::is_base_of<T2, T>::value, "pointer types must be comparable");
    return ptr.get() == raw;
}
template <template <typename...> class Ptr, typename T2, typename... PtrArgs>
bool operator==(T2* raw, Ptr<PtrArgs...>& ptr) {
    return ptr == raw;
}

#endif // SMART_PTR_EQUALITY_HPP
