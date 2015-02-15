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
#include <thread>
#include <mutex>
using std::mutex;
using std::lock_guard;

#include <shared_mutex>
#include <boost/thread/shared_lock_guard.hpp>
using shared_mutex = std::shared_timed_mutex;
template <typename Mutex> using shared_lock = boost::shared_lock_guard<Mutex>;
#include <boost/range/algorithm/sort.hpp>
using namespace boost::range;
#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;
#include <boost/algorithm/hex.hpp>
using boost::algorithm::hex;

#include <openssl/md5.h>

#include <network/uri.hpp>
#include <network/uri/uri_builder.hpp>
#include <network/http/v2/client.hpp>

#include <melosic/melin/logging.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/common/error.hpp>

#include <jbson/json_reader.hpp>

#include "service.hpp"
#include "track.hpp"
#include "user.hpp"
#include "wiki.hpp"
#include "http_client.hpp"
#include "error.hpp"

static Melosic::Logger::Logger logject(logging::keywords::channel = "lastfm::service");

namespace lastfm {

constexpr size_t cstrlen(const char* str) {
    size_t len = 0;
    while(str && *str++)
        ++len;
    return len;
}

#define CONSTEXPR_STRING(str)                                                                                          \
    std::experimental::string_view { str, cstrlen(str) }

static_assert(CONSTEXPR_STRING("").size() == 0, "");
static_assert(CONSTEXPR_STRING(" ").size() == 1, "");
static_assert(CONSTEXPR_STRING("  ").size() == 2, "");

static constexpr auto base_url = CONSTEXPR_STRING("http://ws.audioscrobbler.com/2.0/");

struct service::impl {
    impl(std::string_view api_key, std::string_view shared_secret)
        : null_worker(std::make_unique<asio::io_service::work>(io_service)), client(io_service),
          m_thread([&]() { io_service.run(); }), api_key(api_key), shared_secret(shared_secret) {}

    ~impl() {
        null_worker.reset();
        m_thread.join();
    }

    Method prepareMethodCall(const std::string& methodName) { return Method(methodName); }

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

    user& setUser(user u) {
        lock_guard<Mutex> l(mu);
        return user = std::move(u);
    }

    user& getUser() {
        shared_lock<Mutex> l(mu);
        return user;
    }

    network::http::v0::request make_read_request(std::experimental::string_view method, params_t params) {
        auto format_it = boost::find_if(params, [](auto&& pair) { return std::get<0>(pair) == "format"; });
        if(format_it != std::end(params) && std::get<1>(*format_it) != "json")
            std::get<1>(*format_it) = "json";
        auto api_key_it = boost::find_if(params, [](auto&& pair) { return std::get<0>(pair) == "api_key"; });
        if(api_key_it != std::end(params) && std::get<1>(*api_key_it) != api_key)
            std::get<1>(*api_key_it) = api_key;

        network::uri_builder uri_build{network::uri{std::begin(base_url), std::end(base_url)}};

        uri_build.query("method", method.to_string());
        for(const auto& member :
            boost::adaptors::filter(params, [](auto&& pair) { return std::get<0>(pair) != "method"; })) {
            uri_build.query(std::get<0>(member), std::get<1>(member));
        }
        if(format_it == std::end(params))
            uri_build.query("format", "json");
        if(api_key_it == std::end(params))
            uri_build.query("api_key", api_key);

        network::http::v0::request req{network::uri{uri_build}};
        req.method(network::http::v2::method::get).version("1.0");
        return req;
    }

    asio::io_service io_service;
    std::unique_ptr<asio::io_service::work> null_worker;
    network::http::v0::client client;
    asio::thread m_thread;

    const std::string api_key;
    const std::string shared_secret;
    user user;
    std::shared_ptr<track> currentTrack_;
    typedef shared_mutex Mutex;
    Mutex mu;
    friend class service;
};

service::service(std::string_view api_key, std::string_view shared_secret) : pimpl(new impl(api_key, shared_secret)) {}

service::~service() {}

std::string_view service::api_key() const { return pimpl->api_key; }

std::string_view service::shared_secret() const { return pimpl->shared_secret; }

Method service::prepareMethodCall(const std::string& methodName) {
    return std::move(pimpl->prepareMethodCall(methodName));
}

Method service::sign(Method method) {
    TRACE_LOG(logject) << "Signing method call";
    method.getParameters().front().addMember("sk", getUser().getSessionKey());
    std::deque<Member> members(method.getParameters().front().getMembers().begin(),
                               method.getParameters().front().getMembers().end());
    members.emplace_back("method", method.methodName);
    sort(members, [](const Member& a, const Member& b) { return b.first > a.first; });
    std::string sig;
    for(auto& mem : members) {
        sig += mem.first + mem.second;
    }
    sig += shared_secret().to_string();
    TRACE_LOG(logject) << "sig: " << sig;

    std::array<uint8_t, MD5_DIGEST_LENGTH> sigp;
    MD5(reinterpret_cast<const uint8_t*>(sig.c_str()), sig.length(), sigp.data());
    sig.clear();

    hex(sigp, std::back_inserter(sig));
    TRACE_LOG(logject) << "MD5 sig: " << sig;

    method.getParameters().front().addMember("api_sig", sig);

    return std::move(method);
}

std::future<tag> service::get_tag(std::experimental::string_view tag_name) {
    return get("tag.getinfo", {{"tag", tag_name.to_string()}}, use_future,
               [](network::http::v2::response response) {
        jbson::json_reader reader{};
        reader.parse(response.body());
        auto doc = static_cast<jbson::document>(reader);
        check_error(doc);
        auto elem = doc.find("tag");
        if(elem == doc.end()) {
            throw 0;
        }
        return jbson::get<tag>(*elem);
    });
}

std::string service::postMethod(const Method& method) {
    return {};
    //    return std::move(pimpl->postMethod(method));
}

network::http::v2::request service::make_read_request(std::experimental::string_view method, params_t params) {
    return pimpl->make_read_request(method, std::move(params));
}

std::future<network::http::v2::response> service::get(std::string_view method, params_t params, use_future_t<>) {
    auto req = pimpl->make_read_request(method, std::move(params));
    return pimpl->client.execute(req, {}, network::http::v0::use_future);
}

void service::get(std::string_view method, service::params_t params,
                  std::function<void(asio::error_code, network::http::v2::response)> callback) {
    auto req = pimpl->make_read_request(method, std::move(params));
    return pimpl->client.execute(req, {}, std::move(callback));
}

void service::playlistChangeSlot(std::shared_ptr<Melosic::Core::Playlist> playlist) {
    //    if(playlist && *playlist)
    //        trackChangedSlot(*playlist->currentTrack());
}

void service::trackChangedSlot(const Melosic::Core::Track& newTrack) {
    pimpl->currentTrack_ = std::make_shared<track>(shared_from_this(), newTrack);
}

std::shared_ptr<track> service::currentTrack() { return pimpl->currentTrack_; }

void service::setUser(user u) { pimpl->setUser(std::move(u)); }

user& service::getUser() { return pimpl->getUser(); }

} // namespace lastfm
