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

namespace Melosic {

template <typename T>
struct isStdPtr : std::false_type {};
template <typename T>
struct isStdPtr<std::shared_ptr<T>> : std::true_type {};
template <typename T>
struct isStdPtr<std::weak_ptr<T>> : std::true_type {};

template <typename T>
struct WeakPtrBind {
    typedef T* result_type;
    T* operator()(const std::weak_ptr<T>& ptr) {
        //libstdc++ doesn't throw when it should
        auto sptr = ptr.lock();
        if(sptr)
            return sptr.get();
        throw std::bad_weak_ptr();
    }
};

template <template<class> class Ptr, typename T>
auto bindWeakPtr(Ptr<T> ptr) {
    return std::bind(WeakPtrBind<T>(), std::weak_ptr<T>(ptr));
}

}// Melosic

namespace std {
template <typename T>
struct is_bind_expression<Melosic::WeakPtrBind<T>> : public true_type {};
}

#endif // MELOSIC_PTR_ADAPTOR_HPP
