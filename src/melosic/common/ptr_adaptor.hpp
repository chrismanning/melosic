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

#ifndef MELOSIC_PTR_ADAPTOR_HPP
#define MELOSIC_PTR_ADAPTOR_HPP

namespace Melosic {

template <typename T>
class WeakPtrAdaptor {
public:
    typedef T element_type;

    WeakPtrAdaptor(const std::weak_ptr<T>& ptr) : ptr(ptr) {}

    T* operator->() {
        return std::shared_ptr<T>(ptr).get();
    }

    T& operator*() {
        return *std::shared_ptr<T>(ptr);
    }

private:
    std::weak_ptr<T> ptr;
};

template <typename T>
struct AdaptIfSmartPtr {
    typedef T type;
};
template <typename T>
struct AdaptIfSmartPtr<std::shared_ptr<T>> {
    typedef WeakPtrAdaptor<T> type;
};
template <typename T>
struct AdaptIfSmartPtr<std::weak_ptr<T>> {
    typedef WeakPtrAdaptor<T> type;
};

}

#endif // MELOSIC_PTR_ADAPTOR_HPP
