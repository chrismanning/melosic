// Copyright (C) 2013, 2014 by Glyn Matthews
// Copyright Dean Michael Berris 2012.
// Copyright Google, Inc. 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file
 * \brief An HTTP client and configuration options.
 */

#ifndef NETWORK_HTTP_V0_CLIENT_CLIENT_INC
#define NETWORK_HTTP_V0_CLIENT_CLIENT_INC

#include <future>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <string>
#include <vector>
#include <chrono>

#include <boost/algorithm/string/trim.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/lexical_cast.hpp>

#include <network/config.hpp>
#include <network/version.hpp>
#include <network/http/v2/client/request.hpp>
#include <network/http/v2/client/response.hpp>
#include <network/uri.hpp>
#include <network/config.hpp>
#include <network/http/v2/method.hpp>
#include <network/http/v2/client/request.hpp>
#include <network/http/v2/client/response.hpp>

#include <asio.hpp>
#include <asio/strand.hpp>
#include <asio/deadline_timer.hpp>
#include <asio/use_future.hpp>

#include "exports.hpp"
#include "mfunction.hpp"

namespace network {
namespace http {
namespace v0 {

using v2::client_error;
using v2::make_error_code;
using v2::client_exception;
namespace status = v2::status;
using v2::method;
using asio::use_future_t;
constexpr use_future_t<> use_future{};

class client_options {
  public:
    client_options();

    client_options(const client_options& other) = default;

    client_options(client_options&& other) = default;

    client_options& operator=(client_options other) {
        other.swap(*this);
        return *this;
    }

    ~client_options() = default;

    void swap(client_options& other) noexcept;

    client_options& follow_redirects(bool follow_redirects) {
        m_follow_redirects = follow_redirects;
        return *this;
    }
    bool follow_redirects() const {
        return m_follow_redirects;
    }

    client_options& cache_resolved(bool cache_resolved) {
        m_cache_resolved = cache_resolved;
        return *this;
    }
    bool cache_resolved() const {
        return m_cache_resolved;
    }

    client_options& use_proxy(bool use_proxy) {
        m_use_proxy = use_proxy;
        return *this;
    }
    bool use_proxy() const {
        return m_use_proxy;
    }

    client_options& timeout(std::chrono::milliseconds timeout) {
        m_timeout = timeout;
        return *this;
    }
    std::chrono::milliseconds timeout() const {
        return m_timeout;
    }

    client_options& openssl_certificate_path(std::string path) {
        m_openssl_certificate_paths.emplace_back(std::move(path));
        return *this;
    }
    std::vector<std::string> openssl_certificate_paths() const {
        return m_openssl_certificate_paths;
    }

    client_options& openssl_verify_path(std::string path) {
        m_openssl_verify_paths.emplace_back(std::move(path));
        return *this;
    }
    std::vector<std::string> openssl_verify_paths() const {
        return m_openssl_verify_paths;
    }

    client_options& always_verify_peer(bool always_verify_peer) {
        m_always_verify_peer = always_verify_peer;
        return *this;
    }
    bool always_verify_peer() const {
        return m_always_verify_peer;
    }

    client_options& user_agent(const std::string& user_agent) {
        m_user_agent = user_agent;
        return *this;
    }
    std::string user_agent() const {
        return m_user_agent;
    }

  private:
    bool m_follow_redirects;
    bool m_cache_resolved;
    bool m_use_proxy;
    bool m_always_verify_peer;
    std::string m_user_agent;
    std::chrono::milliseconds m_timeout;
    std::vector<std::string> m_openssl_certificate_paths;
    std::vector<std::string> m_openssl_verify_paths;
};

inline void swap(client_options& lhs, client_options& rhs) noexcept {
    lhs.swap(rhs);
}

typedef v2::client_message::request_options request_options;
typedef v2::client_message::request request;
typedef v2::client_message::response response;

class LASTFM_EXPORT client {
    client(const client&) = delete;
    client& operator=(const client&) = delete;

  public:
    using callback_t = util::mfunction<void(asio::error_code, response)>;

    explicit client(asio::io_service&, client_options options = client_options());

    ~client();

    void execute(request req, request_options options, callback_t callback);
    template <typename TransformerT>
    auto execute(request req, request_options options, use_future_t<>, TransformerT&& transformer);

    std::future<response> execute(request req, request_options options, use_future_t<>);

    std::future<response> get(request req, request_options options, use_future_t<>);

    std::future<response> post(request req, request_options options, use_future_t<>);

    std::future<response> put(request req, request_options options, use_future_t<>);

    std::future<response> delete_(request req, request_options options, use_future_t<>);

    std::future<response> head(request req, request_options options, use_future_t<>);

    std::future<response> options(request req, request_options options, use_future_t<>);

  private:
    asio::io_service& io_service;
    client_options m_options;
};

template <typename TransformerT>
auto client::execute(request req, request_options options, use_future_t<>, TransformerT&& transformer) {
    using transformer_t = std::decay_t<TransformerT>;
    using ret_t = std::result_of_t<transformer_t(response)>;

    struct transformer_wrapper {
        transformer_wrapper(const transformer_wrapper&) = delete;
        transformer_wrapper(transformer_wrapper&&) = default;
        void operator()(asio::error_code ec, response res) const {
            if(ec) {
                m_promise.set_exception(std::make_exception_ptr(std::system_error(ec)));
                return;
            }

            try {
                m_promise.set_value(transform(res));
            } catch(...) {
                m_promise.set_exception(std::current_exception());
            }
        }

        transformer_t transform;
        mutable std::promise<ret_t> m_promise;
    } callback{std::forward<TransformerT>(transformer)};

    auto fut = callback.m_promise.get_future();

    execute(std::move(req), std::move(options), std::move(callback));

    return fut;
}

} // namespace v0
} // namespace http
} // namespace network

#endif // NETWORK_HTTP_V0_CLIENT_CLIENT_INC
