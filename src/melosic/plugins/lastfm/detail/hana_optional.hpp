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

#ifndef LASTFM_HANA_OPTIONAL
#define LASTFM_HANA_OPTIONAL

#include <experimental/optional>

#include <boost/hana.hpp>

namespace std {
using namespace experimental;
}

namespace boost::hana {

    namespace ext::std {
        struct Optional;
    }

    template <typename T> struct datatype<::std::optional<T>> { using type = ext::std::Optional; };

    template <> struct lift_impl<ext::std::Optional> {
        template <typename X> static constexpr decltype(auto) apply(X&& x) {
            return std::make_optional(std::forward<X>(x));
        }
    };

    template <> struct ap_impl<ext::std::Optional> {
        template <typename F, typename... X>
        static constexpr auto apply_impl(std::false_type, F&& f, X&&... x)
            -> decltype(std::make_optional((*f)(x.value()...))) {
            if(f && (... && x))
                return std::make_optional((*f)(x.value()...));
            return std::nullopt;
        }
        template <typename F, typename... X> static constexpr void apply_impl(std::true_type, F&& f, X&&... x) {
            if(f && (... && x))
                (*f)(x.value()...);
        }
        template <typename F, typename... X> static constexpr decltype(auto) apply(F&& f, X&&... x) {
            return apply_impl(std::is_void<decltype((*f)(x.value()...))>{}, std::forward<F>(f), std::forward<X>(x)...);
        }
    };

    template <> struct transform_impl<ext::std::Optional> : Applicative::transform_impl<ext::std::Optional> {};

    template <> struct flatten_impl<ext::std::Optional> {
        template <typename X> static constexpr decltype(auto) apply(std::optional<std::optional<X>>&& x) {
            return x ? *std::move(x) : std::nullopt;
        }

        template <typename X> static constexpr decltype(auto) apply(std::optional<std::optional<X>>& x) {
            return x ? *x : std::nullopt;
        }

        template <typename X> static constexpr decltype(auto) apply(const std::optional<std::optional<X>>& x) {
            return x ? *x : std::nullopt;
        }

        template <typename X> static constexpr decltype(auto) apply(X&& x) {
            return std::forward<X>(x);
        }
    };
}

#endif // LASTFM_HANA_OPTIONAL
