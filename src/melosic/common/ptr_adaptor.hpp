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

template <typename T> struct isWeakPtr : std::false_type {};
template <typename T> struct isWeakPtr<std::weak_ptr<T>> : std::true_type {};
template <typename T> struct isWeakPtr<boost::weak_ptr<T>> : std::true_type {};

template <typename T> struct isSharedPtr : std::false_type {};
template <typename T> struct isSharedPtr<std::shared_ptr<T>> : std::true_type {};
template <typename T> struct isSharedPtr<boost::shared_ptr<T>> : std::true_type {};

template <template <class> class Ptr, typename T> struct WeakPtrBind {
    typedef T object_type;
    static_assert(isWeakPtr<Ptr<object_type>>::value, "WeakPtrBind is for binding weak pointers ONLY");

    explicit WeakPtrBind(Ptr<object_type> ptr) noexcept(std::is_move_constructible<Ptr<object_type>>::value)
        : ptr(std::move(ptr)) {
    }

    auto operator()() const {
        auto locked_ptr = ptr.lock();
        if(!locked_ptr)
            throw std::bad_weak_ptr();
        return locked_ptr;
    }

    object_type& operator*() const {
        return *(*this)();
    }

  private:
    Ptr<object_type> ptr;
};

template <template <class> class Ptr, typename T> struct SharedPtrBind {
    typedef T object_type;
    static_assert(isSharedPtr<Ptr<object_type>>::value, "SharedPtrBind is for binding shared pointers ONLY");

    explicit SharedPtrBind(Ptr<object_type> ptr) noexcept(std::is_move_constructible<Ptr<object_type>>::value)
        : ptr(std::move(ptr)) {
    }

    object_type* operator()() const {
        return ptr.get();
    }

    object_type& operator*() const {
        return *(*this)();
    }

  private:
    Ptr<object_type> ptr;
};

template <typename T> struct RefBind {
    typedef T object_type;

    explicit RefBind(std::reference_wrapper<object_type> wrapper) noexcept : wrapper(wrapper) {
    }

    object_type* operator()() const noexcept {
        return &wrapper.get();
    }

    object_type& operator*() const noexcept {
        return *(*this)();
    }

  private:
    std::reference_wrapper<object_type> wrapper;
};

template <typename T> struct PtrBind {
    typedef T object_type;

    explicit PtrBind(object_type* ptr) noexcept : ptr(ptr) {
    }

    object_type* operator()() const noexcept {
        return ptr;
    }

    object_type& operator*() const noexcept {
        return *(*this)();
    }

  private:
    object_type* ptr;
};

struct keep_alive {};
struct let_it_die {};

template <typename KeepAlive = let_it_die, typename T, class DecayedT = typename std::decay<T>::type>
auto bindObj(std::reference_wrapper<T> obj) {
    static_assert(std::is_class<DecayedT>::value || std::is_pointer<DecayedT>::value,
                  "bindObj must be called with a class object/pointer");
    return RefBind<T>(obj);
}

template <typename KeepAlive = let_it_die, typename T> auto bindObj(T& obj) {
    static_assert(std::is_class<typename std::decay<T>::type>::value, "bindObj must be called with a class object");
    return bindObj(std::ref(obj));
}

template <typename KeepAlive = let_it_die, typename T> auto bindObj(const T& obj) {
    static_assert(std::is_class<typename std::decay<T>::type>::value, "bindObj must be called with a class object");
    return bindObj(std::cref(obj));
}

template <typename KeepAlive = let_it_die, typename T> auto bindObj(T* ptr) {
    return PtrBind<T>(ptr);
}

template <typename KeepAlive = let_it_die, template <class> class Ptr, typename T,
          class = typename std::enable_if<isWeakPtr<Ptr<T>>::value>::type>
auto bindObj(Ptr<T> ptr) {
    return WeakPtrBind<Ptr, T>(std::move(ptr));
}

template <typename KeepAlive = let_it_die, typename T> auto bindObj(boost::shared_ptr<T> ptr) {
    return bindObj(boost::weak_ptr<T>(ptr));
}

template <typename KeepAlive = let_it_die, typename T> auto bindObj(std::shared_ptr<T> ptr) {
    return bindObj(std::weak_ptr<T>(ptr));
}

template <typename KeepAlive, typename T>
auto bindObj(boost::shared_ptr<T> ptr,
             typename std::enable_if<std::is_same<KeepAlive, keep_alive>::value>::type* p = 0) {
    return SharedPtrBind<boost::shared_ptr, T>(std::move(ptr));
}

template <typename KeepAlive, typename T,
          typename = typename std::enable_if<std::is_same<KeepAlive, keep_alive>::value>::type>
auto bindObj(std::shared_ptr<T> ptr, typename std::enable_if<std::is_same<KeepAlive, keep_alive>::value>::type* p = 0) {
    return SharedPtrBind<std::shared_ptr, T>(std::move(ptr));
}

} // Melosic

#endif // MELOSIC_PTR_ADAPTOR_HPP
