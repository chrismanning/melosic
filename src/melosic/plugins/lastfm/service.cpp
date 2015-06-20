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

#include <sstream>
#include <memory>

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/thread/thread.hpp>
using namespace boost::range;
using namespace boost::adaptors;
using boost::algorithm::hex;

#include <openssl/md5.h>

#include <network/uri.hpp>
#include <network/uri/uri_builder.hpp>
#include <network/http/v2/client.hpp>

#include <jbson/document.hpp>
#include <jbson/json_reader.hpp>

#include <lastfm/service.hpp>
#include <lastfm/track.hpp>
#include <lastfm/user.hpp>
#include <lastfm/wiki.hpp>
#include <lastfm/http_client.hpp>
#include <lastfm/error.hpp>

namespace lastfm {

constexpr std::string_view operator""_sv(const char* str, size_t len) {
    return {str, len};
}

static_assert((""_sv).size() == 0, "");
static_assert((" "_sv).size() == 1, "");
static_assert(("  "_sv).size() == 2, "");

static constexpr auto base_url = "http://ws.audioscrobbler.com/2.0/"_sv;

struct service::impl {
    impl(std::string_view api_key, std::string_view shared_secret)
        : null_worker(std::make_unique<asio::io_service::work>(io_service)), client(io_service), api_key(api_key),
          shared_secret(shared_secret) {
        for(auto i = 0u; i < 3; ++i) {
            m_threads.create_thread([&]() { io_service.run(); });
        }
    }

    ~impl() {
        null_worker.reset();
        m_threads.join_all();
    }

    //    std::string postMethod(const Method& method) {
    //        network::uri_builder uri_build;
    //        const Parameter& param = method.getParameters().front();

    ////        std::replace(qstr.begin(), qstr.end(), ' ', '+');
    //        uri_build.scheme("http")
    //           .host("ws.audioscrobbler.com")
    //           .path("/2.0/");
    //        for(const auto& member : param.getMembers()) {
    //            uri_build.query(member.first, member.second);
    //        }
    //        auto uri = network::uri(uri_build);
    //        TRACE_LOG(logject) << "Query uri: " << uri;
    //        request.url(uri);

    //        network::http::v2::response response = client.post(request).get();

    //        std::string str;
    //        try {
    //            auto status = response.status();
    //            TRACE_LOG(logject) << "HTTP response status: " << (int)status;
    //            if(status != network::http::v2::status::code::ok) {
    //                str = response.status_message();
    //                ERROR_LOG(logject) << str;
    ////                BOOST_THROW_EXCEPTION(Melosic::HttpException() << Melosic::ErrorTag::HttpStatus({status, str}));
    //            }

    //            str = response.body();
    //        }
    //        catch(Melosic::HttpException& e) {
    //            str.clear();
    //            std::stringstream ss;
    //            ss << "Network: ";
    //            if(auto httpStatus = boost::get_error_info<Melosic::ErrorTag::HttpStatus>(e))
    //                ss << "http status error: " << httpStatus->status_code << ": " << httpStatus->status_str;
    //            else
    //                ss << "unknown error";
    //            ERROR_LOG(logject) << ss.str();
    //        }
    //        catch(std::future_error& e) {
    //            ERROR_LOG(logject) << "Network: " << e.what();
    //        }
    //        catch(std::runtime_error& e) {
    //            ERROR_LOG(logject) << "Network: " << e.what();
    //        }

    //        request.url(network::uri());

    //        return str;
    //    }

    network::http::v0::request make_read_request(std::experimental::string_view method, params_t params) {
        network::uri_builder uri_build{network::uri{std::begin(base_url), std::end(base_url)}};

        uri_build.query("method", method.to_string());
        using namespace boost::adaptors;
        for(const auto& member : params | filtered([](auto&& pair) { return std::get<0>(pair) != "method"; })) {
            std::string in = std::get<1>(member), out;
            network::uri::encode_query(in.begin(), in.end(), std::back_inserter(out));
            uri_build.query(std::get<0>(member), std::move(out));
        }
        uri_build.query("format", "json");
        uri_build.query("api_key", api_key);

        network::http::v0::request req{network::uri{uri_build}};
        req.method(network::http::v2::method::get).version("1.0");
        return req;
    }

    asio::io_service io_service;
    std::unique_ptr<asio::io_service::work> null_worker;
    network::http::v0::client client;
    boost::thread_group m_threads;

    const std::string api_key;
    const std::string shared_secret;
    user user;
    friend class service;
};

service::service(std::string_view api_key, std::string_view shared_secret) : pimpl(new impl(api_key, shared_secret)) {
}

service::~service() {
}

std::string_view service::api_key() const {
    return pimpl->api_key;
}

std::string_view service::shared_secret() const {
    return pimpl->shared_secret;
}

// Method service::sign(Method method) {
//    TRACE_LOG(logject) << "Signing method call";
//    method.getParameters().front().addMember("sk", getUser().getSessionKey());
//    std::deque<Member> members(method.getParameters().front().getMembers().begin(),
//                               method.getParameters().front().getMembers().end());
//    members.emplace_back("method", method.methodName);
//    sort(members, [](const Member& a, const Member& b) { return b.first > a.first; });
//    std::string sig;
//    for(auto& mem : members) {
//        sig += mem.first + mem.second;
//    }
//    sig += shared_secret().to_string();
//    TRACE_LOG(logject) << "sig: " << sig;

//    std::array<uint8_t, MD5_DIGEST_LENGTH> sigp;
//    MD5(reinterpret_cast<const uint8_t*>(sig.c_str()), sig.length(), sigp.data());
//    sig.clear();

//    hex(sigp, std::back_inserter(sig));
//    TRACE_LOG(logject) << "MD5 sig: " << sig;

//    method.getParameters().front().addMember("api_sig", sig);

//    return std::move(method);
//}

void service::get(std::string_view method, service::params_t params,
                  util::mfunction<void(asio::error_code, std::string)> callback) {
    auto req = pimpl->make_read_request(method, std::move(params));
    return pimpl->client.execute(std::move(req), {}, [c = std::move(callback)](auto error_code, auto response) mutable {
        return c(error_code, response.body());
    });
}

jbson::document service::document_callback(asio::error_code ec, std::string body) {
    if(ec) {
        BOOST_THROW_EXCEPTION(std::system_error(ec));
    }
    auto doc = jbson::read_json(body);
    check_error(doc);
    return doc;
}

} // namespace lastfm
