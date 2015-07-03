/**************************************************************************
**  Copyright (C) 2012 Christian Manning
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

#ifndef LASTFM_SERVICE_HPP
#define LASTFM_SERVICE_HPP

#include <string>
#include <memory>
#include <vector>
#include <experimental/string_view>

#include <boost/uuid/uuid_io.hpp>
#include <boost/hana/ext/std.hpp>

#include <pplx/pplxtasks.h>

#include <jbson/document.hpp>

#include <lastfm/lastfm.hpp>
#include <lastfm/detail/hana_optional.hpp>
#include <lastfm/detail/transform.hpp>
#include <lastfm/mfunction.hpp>

namespace hana = boost::hana;

namespace lastfm {

struct user;
struct track;
struct tag;
struct artist;

class LASTFM_EXPORT service {
  public:
    using params_t = std::vector<std::tuple<std::string, std::string>>;

    explicit service(std::string_view api_key, std::string_view shared_secret);

    ~service();

    std::string_view api_key() const;
    std::string_view shared_secret() const;

    pplx::task<jbson::document> get(std::string_view method, params_t params);

    template <typename TransformerT, typename ReturnT = std::result_of_t<TransformerT(jbson::document)>>
    pplx::task<ReturnT> get(std::string_view method, params_t params, TransformerT&& transform);

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

template <typename TransformerT, typename ReturnT>
pplx::task<ReturnT> service::get(std::string_view method, params_t params, TransformerT&& transform) {
    return get(method, std::move(params)).then(transform);
}

namespace detail {

struct to_string_ {
    inline std::string operator()(date_t t) const {
        auto time = date_t::clock::to_time_t(t);
        std::stringstream ss{};
        ss << std::put_time(std::gmtime(&time), "%a, %d %b %Y %H:%M:%S");
        return ss.str();
    }
    inline std::string operator()(std::string_view s) const {
        return s.to_string();
    }
    inline std::string operator()(std::string s) const {
        return std::move(s);
    }
    template <typename N> inline std::string operator()(N&& i) const {
        using ::std::to_string;
        return to_string(i);
    }
};

struct make_params_ {
    template <typename... PairT> service::params_t operator()(PairT&&... optional_params) const {
        service::params_t params{};
        for(auto&& param : {make_param(std::forward<PairT>(optional_params))...}) {
            hana::unpack(std::forward<decltype(param)>(param), [&](auto&& name, auto&& maybe) {
                if(maybe && !maybe->empty())
                    params.emplace_back(std::move(name), *std::move(maybe));
            });
        }
        return params;
    }

  private:
    template <typename PairT> static std::tuple<std::string, std::optional<std::string>> make_param(PairT&& t) {
        return hana::unpack(std::forward<PairT>(t), [](auto&& a, auto&& b) {
            return std::make_tuple(std::string{a}, hana::transform(hana::flatten(std::make_optional(b)), to_string_{}));
        });
    }
};

constexpr auto make_params = make_params_{};

} // namespace detail

} // namespace lastfm

#endif // LASTFM_SERVICE_HPP
