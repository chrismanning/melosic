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
    client_options()
        : m_follow_redirects(false), m_cache_resolved(false), m_use_proxy(false), m_always_verify_peer(false),
          m_user_agent(std::string("cpp-netlib/") + NETLIB_VERSION), m_timeout(30000) {}

    client_options(const client_options& other) = default;

    client_options(client_options&& other) = default;

    client_options& operator=(client_options other) {
        other.swap(*this);
        return *this;
    }

    ~client_options() = default;

    void swap(client_options& other) noexcept {
        using std::swap;
        swap(m_follow_redirects, other.m_follow_redirects);
        swap(m_cache_resolved, other.m_cache_resolved);
        swap(m_use_proxy, other.m_use_proxy);
        swap(m_always_verify_peer, other.m_always_verify_peer);
        swap(m_user_agent, other.m_user_agent);
        swap(m_timeout, other.m_timeout);
        swap(m_openssl_certificate_paths, other.m_openssl_certificate_paths);
        swap(m_openssl_verify_paths, other.m_openssl_verify_paths);
    }

    client_options& follow_redirects(bool follow_redirects) {
        m_follow_redirects = follow_redirects;
        return *this;
    }
    bool follow_redirects() const { return m_follow_redirects; }

    client_options& cache_resolved(bool cache_resolved) {
        m_cache_resolved = cache_resolved;
        return *this;
    }
    bool cache_resolved() const { return m_cache_resolved; }

    client_options& use_proxy(bool use_proxy) {
        m_use_proxy = use_proxy;
        return *this;
    }
    bool use_proxy() const { return m_use_proxy; }

    client_options& timeout(std::chrono::milliseconds timeout) {
        m_timeout = timeout;
        return *this;
    }
    std::chrono::milliseconds timeout() const { return m_timeout; }

    client_options& openssl_certificate_path(std::string path) {
        m_openssl_certificate_paths.emplace_back(std::move(path));
        return *this;
    }
    std::vector<std::string> openssl_certificate_paths() const { return m_openssl_certificate_paths; }

    client_options& openssl_verify_path(std::string path) {
        m_openssl_verify_paths.emplace_back(std::move(path));
        return *this;
    }
    std::vector<std::string> openssl_verify_paths() const { return m_openssl_verify_paths; }

    client_options& always_verify_peer(bool always_verify_peer) {
        m_always_verify_peer = always_verify_peer;
        return *this;
    }
    bool always_verify_peer() const { return m_always_verify_peer; }

    client_options& user_agent(const std::string& user_agent) {
        m_user_agent = user_agent;
        return *this;
    }
    std::string user_agent() const { return m_user_agent; }

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

inline void swap(client_options& lhs, client_options& rhs) noexcept { lhs.swap(rhs); }

typedef v2::client_message::request_options request_options;
typedef v2::client_message::request request;
typedef v2::client_message::response response;

struct identity_t {
    template <typename T> auto operator()(T&& in) const { return std::forward<T>(in); }
};

class LASTFM_EXPORT client {
    client(const client&) = delete;
    client& operator=(const client&) = delete;

  public:
    typedef request::string_type string_type;

    explicit client(asio::io_service&, client_options options = client_options());

    ~client();

    template <typename CallbackT,
              typename = std::enable_if_t<!std::is_same<use_future_t<>, std::decay_t<CallbackT>>::value>>
    void execute(request req, request_options options, CallbackT&& callback);
    template <typename TransformerT>
    auto execute(request req, request_options options, use_future_t<>, TransformerT&& transformer);

    inline std::future<response> execute(request req, request_options options, use_future_t<>);

    inline std::future<response> get(request req, request_options options, use_future_t<>);

    inline std::future<response> post(request req, request_options options, use_future_t<>);

    inline std::future<response> put(request req, request_options options, use_future_t<>);

    inline std::future<response> delete_(request req, request_options options, use_future_t<>);

    inline std::future<response> head(request req, request_options options, use_future_t<>);

    inline std::future<response> options(request req, request_options options, use_future_t<>);

  private:
    template <typename> class impl;
    asio::io_service& io_service;
    client_options m_options;
};

using asio::ip::tcp;

struct async_connection {
    explicit async_connection(asio::io_service& io_service) : io_service(io_service) {}

    template <typename Handler> void async_connect(const tcp::resolver::iterator& endpoint_it, Handler&& handler) {
        socket = std::make_unique<tcp::socket>(io_service);
        asio::async_connect(*socket, endpoint_it, std::forward<Handler>(handler));
    }

    template <typename Handler> void async_write(asio::streambuf& command_streambuf, Handler&& handler) {
        asio::async_write(*socket, command_streambuf, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_read_until(asio::streambuf& command_streambuf, const std::string& delim, Handler&& handler) {
        asio::async_read_until(*socket, command_streambuf, delim, std::forward<Handler>(handler));
    }

    template <typename Handler> void async_read(asio::streambuf& command_streambuf, Handler&& handler) {
        asio::async_read(*socket, command_streambuf, asio::transfer_at_least(1), std::forward<Handler>(handler));
    }

    void disconnect() {
        if(socket && socket->is_open()) {
            asio::error_code ec;
            socket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            if(!ec) {
                socket->close(ec);
            }
        }
    }

    void cancel() { socket->cancel(); }

  private:
    asio::io_service& io_service;
    std::unique_ptr<asio::ip::tcp::socket> socket;
};

// I don't want to to delimit with newlines when using the input
// stream operator, so that's why I wrote this function.
LASTFM_EXPORT std::istream& getline_with_newline(std::istream& is, std::string& line);

template <typename CallbackT>
class client::impl : public boost::intrusive_ref_counter<impl<CallbackT>, boost::thread_safe_counter> {
    using self_t = impl<CallbackT>;
    using callback_t = std::decay_t<CallbackT>;
    static_assert(std::is_void<std::result_of_t<callback_t(asio::error_code, response)>>::value, "");

    template <typename T>
    explicit impl(asio::io_service& io_service, client_options client_opts, request req, request_options request_opts,
                  T&& callback)
        : io_service(io_service), m_client_options(client_opts), m_resolver(io_service), m_timedout(false),
          m_timer(io_service), m_connection(std::make_shared<async_connection>(io_service)), m_request(req),
          m_request_options(request_opts), callback(std::forward<T>(callback)) {}

    auto shared_from_this() { return boost::intrusive_ptr<self_t>{this}; }
    auto shared_from_this() const { return boost::intrusive_ptr<const self_t>{this}; }

    void set_error(const asio::error_code& ec) {
        callback(ec, response{});
        m_timer.cancel();
    }

    void execute() {
        // If there is no user-agent, provide one as a default.
        if(!m_request.header("User-Agent")) {
            m_request.append_header("User-Agent", m_client_options.user_agent());
        }
        if(m_request.version().empty()) {
            m_request.version("1.1");
        }

        // Get the host and port from the request and resolve
        auto url = m_request.url();
        auto host = url.host().value_or("").to_string();
        auto port = url.port().value_or("80").to_string();

        asio::ip::tcp::resolver::query query{host, port};
        m_resolver.async_resolve(query, [self = this->shared_from_this()](const asio::error_code& ec,
                                                                          tcp::resolver::iterator endpoint_iterator) {
            self->connect(ec, endpoint_iterator);
        });

        if(m_client_options.timeout() > std::chrono::milliseconds(0)) {
            m_timer.expires_from_now(boost::posix_time::milliseconds(m_client_options.timeout().count()));
            m_timer.async_wait([self = this->shared_from_this()](const asio::error_code& ec) { self->timeout(ec); });
        }
    }

    void timeout(const asio::error_code& ec) {
        if(!ec) {
            m_connection->disconnect();
        }
        m_timedout = true;
    }

    void connect(const asio::error_code& ec, tcp::resolver::iterator endpoint_iterator) {
        if(ec) {
            set_error(ec);
            return;
        }

        // make a connection to an endpoint
        m_connection->async_connect(
            endpoint_iterator,
            [self = this->shared_from_this()](const asio::error_code& ec, auto&&) { self->write_request(ec); });
    }

    void write_request(const asio::error_code& ec) {
        if(m_timedout) {
            set_error(asio::error::timed_out);
            return;
        }

        if(ec) {
            set_error(ec);
            return;
        }

        // write the request to an I/O stream.
        std::ostream request_stream(&m_request_buffer);
        request_stream << m_request;
        if(!request_stream) {
            set_error(make_error_code(client_error::invalid_request));
            return;
        }

        m_connection->async_write(
            m_request_buffer, [self = this->shared_from_this()](const asio::error_code& ec, std::size_t bytes_written) {
                self->write_body(ec, bytes_written);
            });
    }

    void write_body(const asio::error_code& ec, std::size_t bytes_written) {
        if(m_timedout) {
            set_error(asio::error::timed_out);
            return;
        }

        if(ec) {
            set_error(ec);
            return;
        }

        // update progress
        //    context->total_bytes_written_ += bytes_written;
        //    if(auto progress = context->options_.progress()) {
        //        progress(client_message::transfer_direction::bytes_written, context->total_bytes_written_);
        //    }

        // write the body to an I/O stream
        std::ostream request_stream(&m_request_buffer);
        // TODO write payload to request_buffer_
        if(!request_stream) {
            set_error(make_error_code(client_error::invalid_request));
            return;
        }

        m_connection->async_write(
            m_request_buffer, [self = this->shared_from_this()](const asio::error_code& ec, std::size_t bytes_written) {
                self->read_response(ec, bytes_written);
            });
    }

    void read_response(const asio::error_code& ec, std::size_t bytes_written) {
        if(m_timedout) {
            set_error(asio::error::timed_out);
            return;
        }

        if(ec) {
            set_error(ec);
            return;
        }

        // Create a response object and fill it with the status from the server.
        m_connection->async_read_until(
            m_response_buffer, "\r\n",
            [self = this->shared_from_this()](const asio::error_code& ec, std::size_t bytes_read) {
                self->read_response_status(ec, bytes_read);
            });
    }

    void read_response_status(const asio::error_code& ec, std::size_t bytes_written) {
        if(m_timedout) {
            set_error(asio::error::timed_out);
            return;
        }

        if(ec) {
            set_error(ec);
            return;
        }

        // Update the reponse status.
        std::istream is(&m_response_buffer);
        string_type version;
        is >> version;
        unsigned int status;
        is >> status;
        string_type message;
        std::getline(is, message);

        // if options_.follow_redirects()
        // and if status in range 300 - 307
        // then take the request and reset the URL from the 'Location' header
        // restart connection
        // Not that the 'Location' header can be a *relative* URI

        auto res = std::make_shared<response>();
        res->set_version(version);
        res->set_status(status::code(status));
        res->set_status_message(boost::trim_copy(message));

        // Read the response headers.
        m_connection->async_read_until(
            m_response_buffer, "\r\n\r\n",
            [ res, self = this->shared_from_this() ](const asio::error_code& ec, std::size_t bytes_read) {
                self->read_response_headers(ec, bytes_read, res);
            });
    }

    void read_response_headers(const asio::error_code& ec, std::size_t bytes_read, std::shared_ptr<response> res) {
        if(m_timedout) {
            set_error(asio::error::timed_out);
            return;
        }

        if(ec) {
            set_error(ec);
            return;
        }

        // fill headers
        std::istream is(&m_response_buffer);
        string_type header;
        while(std::getline(is, header) && (header != "\r")) {
            auto value = boost::find<boost::return_next_end>(header, ':');
            auto key = boost::make_iterator_range(std::begin(header), std::prev(std::begin(value)));
            res->add_header(boost::lexical_cast<string_type>(key),
                            boost::lexical_cast<string_type>(boost::trim_copy(value)));
        }

        // read the response body.
        m_connection->async_read(m_response_buffer, [ res, self = this->shared_from_this() ](const asio::error_code& ec,
                                                                                             std::size_t bytes_read) {
            self->read_response_body(ec, bytes_read, res);
        });
    }

    void read_response_body(asio::error_code ec, std::size_t bytes_read, std::shared_ptr<response> res) {
        if(m_timedout) {
            set_error(asio::error::timed_out);
            return;
        }

        if(ec && ec != asio::error::eof) {
            set_error(ec);
            return;
        }
        ec.clear();

        // If there's no data else to read, then set the response and exit.
        if(bytes_read == 0) {
            callback(ec, std::move(*res));
            m_timer.cancel();
            return;
        }

        std::istream is(&m_response_buffer);
        string_type line;
        line.reserve(bytes_read);
        while(!getline_with_newline(is, line).eof()) {
            res->append_body(std::move(line));
        }

        // Keep reading the response body until we have nothing else to read.
        m_connection->async_read(m_response_buffer, [ res, self = this->shared_from_this() ](const asio::error_code& ec,
                                                                                             std::size_t bytes_read) {
            self->read_response_body(ec, bytes_read, res);
        });
    }

    asio::io_service& io_service;
    client_options m_client_options;
    asio::ip::tcp::resolver m_resolver;
    bool m_timedout;
    asio::deadline_timer m_timer;

    std::shared_ptr<async_connection> m_connection;

    request m_request;
    request_options m_request_options;

    asio::streambuf m_request_buffer;
    asio::streambuf m_response_buffer;

    callback_t callback;

    friend class client;
};

template <typename CallbackT, typename>
void client::execute(request req, request_options options, CallbackT&& callback) {
    static_assert(!std::is_same<use_future_t<>, std::decay_t<CallbackT>>::value, "");
    using impl_t = impl<std::decay_t<CallbackT>>;
    auto pimpl = boost::intrusive_ptr<impl_t>{
        new impl_t(io_service, m_options, std::move(req), std::move(options), std::forward<CallbackT>(callback))};
    pimpl->execute();
}

template <typename TransformerT>
auto client::execute(request req, request_options options, use_future_t<>, TransformerT&& transformer) {
    using transformer_t = std::decay_t<TransformerT>;
    using ret_t = std::result_of_t<transformer_t(response)>;

    struct transformer_wrapper {
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

std::future<response> client::execute(request req, request_options options, use_future_t<>) {
    return execute(std::move(req), std::move(options), use_future, identity_t{});
}

std::future<response> client::get(request req, request_options options, use_future_t<>) {
    req.method(method::get);
    return execute(req, options, use_future);
}

std::future<response> client::post(request req, request_options options, use_future_t<>) {
    req.method(method::post);
    return execute(req, options, use_future);
}

std::future<response> client::put(request req, request_options options, use_future_t<>) {
    req.method(method::put);
    return execute(req, options, use_future);
}

std::future<response> client::delete_(request req, request_options options, use_future_t<>) {
    req.method(method::delete_);
    return execute(req, options, use_future);
}

std::future<response> client::head(request req, request_options options, use_future_t<>) {
    req.method(method::head);
    return execute(req, options, use_future);
}

std::future<response> client::options(request req, request_options options, use_future_t<>) {
    req.method(method::options);
    return execute(req, options, use_future);
}

} // namespace v0
} // namespace http
} // namespace network

#endif // NETWORK_HTTP_V3_CLIENT_CLIENT_INC
