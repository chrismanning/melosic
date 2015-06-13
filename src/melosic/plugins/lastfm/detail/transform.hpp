/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#ifndef LASTFM_TRANSFORM
#define LASTFM_TRANSFORM

#include <algorithm>
#include <type_traits>

#include <jbson/element.hpp>
#include <jbson/document.hpp>
#include <jbson/path.hpp>

namespace lastfm {

namespace detail {

template <typename T>
struct deserialise_ {
    template <typename ElemT>
    T operator()(ElemT&& elem) const {
        return jbson::get<T>(elem);
    }
};

} // namespace detail

template <typename T>
constexpr auto deserialise = detail::deserialise_<T>{};

namespace detail {

struct transform_copy_ {
    template <typename V, typename F>
    auto operator()(V&& v, F&& f) const {
        using U = std::remove_cv_t<std::remove_reference_t<decltype(f(*v.begin()))>>;
        using Alloc = typename std::remove_reference_t<V>::allocator_type;
        using NewAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<U>;
        std::vector<U, NewAlloc> result;
        result.reserve(v.size());

        std::transform(std::begin(v), std::end(v), std::back_inserter(result), std::forward<F>(f));
        return result;
    }
};

} // namespace detail

constexpr auto transform_copy = detail::transform_copy_{};

namespace detail {

struct vector_to_optional_ {
    template <typename T, typename... Args> std::optional<T> operator()(std::vector<T, Args...>&& vec) const {
        return vec.empty() ? std::nullopt : std::make_optional(std::move(vec.front()));
    }

    template <typename T, typename... Args> std::optional<T> operator()(const std::vector<T, Args...>& vec) const {
        return vec.empty() ? std::nullopt : std::make_optional(vec.front());
    }
};

} // namespace detail

constexpr auto vector_to_optional = detail::vector_to_optional_{};

namespace detail {

template <typename T>
struct transform_select_ {
    auto operator()(std::string_view path) const {
        return [path = path.to_string()](auto&& doc) {
            if(auto elem = vector_to_optional(jbson::path_select(std::forward<decltype(doc)>(doc), path))) {
                return jbson::get<T>(*elem);
            }
            throw std::runtime_error("invalid response");
        };
    }
};

template <typename T>
struct transform_select_<std::vector<T>> {
    auto operator()(std::string_view path) const {
        return [path = path.to_string()](auto&& doc) {
            auto elems = jbson::path_select(std::forward<decltype(doc)>(doc), path);
            return transform_copy(elems, deserialise<T>);
        };
    }
};

} // namespace detail

template <typename T>
constexpr auto transform_select = detail::transform_select_<T>{};

} // namespace lastfm

#endif // LASTFM_TRANSFORM

