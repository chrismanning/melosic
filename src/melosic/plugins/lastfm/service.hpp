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
#include <future>
#include <experimental/string_view>

#include <boost/uuid/uuid_io.hpp>
#include <boost/hana/ext/std.hpp>

#include <asio/package.hpp>
#include <asio/post.hpp>
#include <asio/use_future.hpp>

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
using asio::use_future_t;
constexpr use_future_t<> use_future{};

class LASTFM_EXPORT service {
  public:
    using params_t = std::vector<std::tuple<std::string, std::string>>;

    explicit service(std::string_view api_key, std::string_view shared_secret);

    ~service();

    std::string_view api_key() const;
    std::string_view shared_secret() const;

    template <typename TransformerT, typename ReturnT = std::result_of_t<TransformerT(jbson::document)>>
    std::future<ReturnT> get(std::string_view method, params_t params, use_future_t<>, TransformerT&& transform);

    void get(std::string_view method, params_t params, util::mfunction<void(asio::error_code, std::string)> callback);

    template <typename T, typename ContinueT> auto then(std::future<T> fut, ContinueT&& continuation);

  private:
    static jbson::document document_callback(asio::error_code, std::string);

    struct impl;
    std::unique_ptr<impl> pimpl;
};

template <typename T, typename ContinueT> auto service::then(std::future<T> fut, ContinueT&& continuation) {
    return asio::post(
        asio::package([ fut = std::move(fut), continuation = std::forward<ContinueT>(continuation) ]() mutable {
            fut.wait();
            return continuation(std::move(fut));
        }));
}

template <typename TransformerT, typename ReturnT>
std::future<ReturnT> service::get(std::string_view method, params_t params, use_future_t<>, TransformerT&& transform) {
    using transformer_t = std::decay_t<TransformerT>;
    using ret_t = ReturnT;

    struct transformer_wrapper {
        void operator()(asio::error_code ec, std::string body) const {
            if(ec) {
                m_promise->set_exception(std::make_exception_ptr(std::system_error(ec)));
                return;
            }

            try {
                auto doc = document_callback(ec, std::move(body));

                m_promise->set_value(transform(doc));
            } catch(...) {
                m_promise->set_exception(std::current_exception());
            }
        }

        transformer_t transform;
        mutable std::shared_ptr<std::promise<ret_t>> m_promise{std::make_shared<std::promise<ret_t>>()};
    } callback{std::forward<TransformerT>(transform)};

    auto fut = callback.m_promise->get_future();

    get(method, std::move(params), std::move(callback));

    return fut;
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
        service::params_t params;
        for(auto&& param : {make_param(optional_params)...}) {
            auto& name = std::get<0>(param);
            auto& maybe = std::get<1>(param);
            if(maybe && !maybe->empty())
                params.emplace_back(name, *maybe);
        }
        return params;
    }

  private:
    template <typename PairT> static decltype(auto) make_param(PairT&& t) {
        return hana::unpack(std::forward<PairT>(t), [](auto&& a, auto&& b) {
            return std::make_tuple(std::string{a}, hana::transform(hana::flatten(std::make_optional(b)), to_string_{}));
        });
    }
};

} // namespace detail

constexpr auto make_params = detail::make_params_{};

} // namespace lastfm

#endif // LASTFM_SERVICE_HPP
