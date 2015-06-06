// Copyright (C) 2013, 2014 by Glyn Matthews
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "http_client.hpp"

namespace network {
namespace http {
namespace v0 {

client::client(asio::io_service& io_service, client_options options) : io_service(io_service), m_options(options) {
}

client::~client() {
}

std::istream& getline_with_newline(std::istream& is, std::string& line) {
    line.clear();

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    while(true) {
        int c = sb->sbumpc();
        switch(c) {
            case EOF:
                if(line.empty()) {
                    is.setstate(std::ios::eofbit);
                }
                return is;
            default:
                line += static_cast<char>(c);
        }
    }
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
