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

} // namespace v3
} // namespace http
} // namespace network
