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

#include <memory>
#include <functional>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

namespace Melosic {

template <typename T>
struct isSharedPtr : std::false_type {};
template <typename T>
struct isSharedPtr<std::shared_ptr<T>> : std::true_type {};
template <typename T>
struct isSharedPtr<boost::shared_ptr<T>> : std::true_type {};

template <typename T>
struct isWeakPtr : std::false_type {};
template <typename T>
struct isWeakPtr<std::weak_ptr<T>> : std::true_type {};
template <typename T>
struct isWeakPtr<boost::weak_ptr<T>> : std::true_type {};

template <typename T>
struct WeakPtrBind {
    typedef T* result_type;

    template <template<class> class Ptr>
    T* operator()(const Ptr<T>& ptr) {
        static_assert(isWeakPtr<Ptr<T>>::value, "WeakPtrBind is for binding weak pointers ONLY");
        return call_impl(ptr);
    }

private:
    template <template<class> class WeakPtr>
    inline T* call_impl(const WeakPtr<T>& ptr) {
        if(ptr.expired())
            throw std::bad_weak_ptr();
        return ptr.lock().get();
    }
};

template <typename>
void bindWeakPtr();

template <typename T, class = typename std::enable_if<std::is_pointer<T>::value>::type>
auto bindWeakPtr(T ptr) {
    return ptr;
}

template <template<class> class Ptr, typename T, class = typename std::enable_if<isWeakPtr<Ptr<T>>::value>::type>
auto bindWeakPtr(const Ptr<T>& ptr) {
    return std::bind(WeakPtrBind<T>(), ptr);
}

template <typename T>
auto bindWeakPtr(boost::shared_ptr<T> ptr) {
    return bindWeakPtr(boost::weak_ptr<T>(ptr));
}

template <typename T>
auto bindWeakPtr(std::shared_ptr<T> ptr) {
    return bindWeakPtr(std::weak_ptr<T>(ptr));
}

}// Melosic

#endif // MELOSIC_PTR_ADAPTOR_HPP
