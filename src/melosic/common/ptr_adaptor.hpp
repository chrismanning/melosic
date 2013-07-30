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
#include <type_traits>

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

template <template<class> class Ptr, typename T>
struct WeakPtrBind {
    static_assert(isWeakPtr<Ptr<T>>::value, "WeakPtrBind is for binding weak pointers ONLY");
    typedef T* result_type;

    explicit WeakPtrBind(const Ptr<T>& ptr) noexcept : ptr(ptr) {}

    T* operator()() const {
        if(ptr.expired())
            throw std::bad_weak_ptr();
        return ptr.lock().get();
    }

    T& operator*() const {
        return *(*this)();
    }

private:
    Ptr<T> ptr;
};

template <typename T>
struct RefBind {
    typedef T* result_type;

    explicit RefBind(const std::reference_wrapper<T>& wrapper) noexcept : wrapper(wrapper) {}

    T* operator()() const noexcept {
        return &wrapper.get();
    }

    T& operator*() const {
        return *(*this)();
    }

private:
    std::reference_wrapper<T> wrapper;
};

template <typename T>
struct PtrBind {
    typedef T* result_type;

    explicit PtrBind(T* ptr) noexcept : ptr(ptr) {}

    T* operator()() const noexcept {
        return ptr;
    }

    T& operator*() const {
        return *(*this)();
    }

private:
    T* ptr;
};

template <typename>
void bindObj();

template <typename T, class DecayedT = typename std::decay<T>::type>
auto bindObj(std::reference_wrapper<T> obj) {
    static_assert(std::is_class<DecayedT>::value || std::is_pointer<DecayedT>::value,
                  "bindObj must be called with a class object/pointer");
    return RefBind<T>(obj);
}

template <typename T>
auto bindObj(T& obj) {
    static_assert(std::is_class<typename std::decay<T>::type>::value,
                  "bindObj must be called with a class object");
    return bindObj(std::ref(obj));
}

template <typename T>
auto bindObj(T* ptr) {
    return PtrBind<T>(ptr);
}

template <template<class> class Ptr, typename T, class = typename std::enable_if<isWeakPtr<Ptr<T>>::value>::type>
auto bindObj(const Ptr<T>& ptr) {
    return WeakPtrBind<Ptr, T>(ptr);
}

template <typename T>
auto bindObj(boost::shared_ptr<T> ptr) {
    return bindObj(boost::weak_ptr<T>(ptr));
}

template <typename T>
auto bindObj(std::shared_ptr<T> ptr) {
    return bindObj(std::weak_ptr<T>(ptr));
}

}// Melosic

#endif // MELOSIC_PTR_ADAPTOR_HPP
