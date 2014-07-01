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
using std::mutex; using std::lock_guard;

#include <shared_mutex>
#include <boost/thread/shared_lock_guard.hpp>
using shared_mutex = std::shared_timed_mutex;
template <typename Mutex>
using shared_lock = boost::shared_lock_guard<Mutex>;
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

#include "service.hpp"
#include "track.hpp"
#include "user.hpp"

static Melosic::Logger::Logger logject(logging::keywords::channel = "LastFM::Service");

namespace LastFM {

class Service::impl {
public:
    impl(const std::string& apiKey, const std::string& sharedSecret)
        : apiKey(apiKey),
          sharedSecret(sharedSecret)
    {}

    Method prepareMethodCall(const std::string& methodName) {
        return Method(methodName);
    }

    std::string postMethod(const Method& method) {
        std::stringstream strstr;
        network::uri_builder uri_build;
        const Parameter& param = method.getParameters().front();

        strstr << "method=" << method.methodName;
        for(const auto& member : param.getMembers()) {
            strstr << "&" << member.first << "=" << member.second;
        }
        std::string qstr(strstr.str());
        std::replace(qstr.begin(), qstr.end(), ' ', '+');
        uri_build.scheme("http")
           .host("ws.audioscrobbler.com")
           .path("/2.0/")
           .query(qstr);
        auto uri = network::uri(uri_build);
        TRACE_LOG(logject) << "Query uri: " << uri;
        request.url(uri);

        network::http::v2::response response = client.post(request).get();

        std::string str;
        try {
            auto status = response.status();
            TRACE_LOG(logject) << "HTTP response status: " << (int)status;
            if(status != network::http::v2::status::code::ok) {
                str = response.status_message();
                ERROR_LOG(logject) << str;
//                BOOST_THROW_EXCEPTION(Melosic::HttpException() << Melosic::ErrorTag::HttpStatus({status, str}));
            }

            str = response.body();
        }
        catch(Melosic::HttpException& e) {
            str.clear();
            std::stringstream ss;
            ss << "Network: ";
            if(auto httpStatus = boost::get_error_info<Melosic::ErrorTag::HttpStatus>(e))
                ss << "http status error: " << httpStatus->status_code << ": " << httpStatus->status_str;
            else
                ss << "unknown error";
            ERROR_LOG(logject) << ss.str();
        }
        catch(std::future_error& e) {
            ERROR_LOG(logject) << "Network: " << e.what();
        }
        catch(std::runtime_error& e) {
            ERROR_LOG(logject) << "Network: " << e.what();
        }

        request.url(network::uri());

        return str;
    }

    User& setUser(User u) {
        lock_guard<Mutex> l(mu);
        return user = std::move(u);
    }

    User& getUser() {
        shared_lock<Mutex> l(mu);
        return user;
    }

private:
    network::http::v2::client client;
    network::http::v2::request request;
    const std::string apiKey;
    const std::string sharedSecret;
    User user;
    std::shared_ptr<Track> currentTrack_;
    typedef shared_mutex Mutex;
    Mutex mu;
    friend class Service;
};

Service::Service(const std::string& apiKey, const std::string& sharedSecret)
    : pimpl(new impl(apiKey, sharedSecret)) {}

Service::~Service() {}

const std::string& Service::apiKey() {
    return pimpl->apiKey;
}

const std::string& Service::sharedSecret() {
    return pimpl->sharedSecret;
}

Method Service::prepareMethodCall(const std::string& methodName) {
    return std::move(pimpl->prepareMethodCall(methodName));
}

Method Service::sign(Method method) {
    TRACE_LOG(logject) << "Signing method call";
    method.getParameters().front().addMember("sk", getUser().getSessionKey());
    std::deque<Member> members(method.getParameters().front().getMembers().begin(),
                               method.getParameters().front().getMembers().end());
    members.emplace_back("method", method.methodName);
    sort(members, [](const Member& a, const Member& b) {
        return b.first > a.first;
    });
    std::string sig;
    for(auto& mem : members) {
        sig += mem.first + mem.second;
    }
    sig += sharedSecret();
    TRACE_LOG(logject) << "sig: " << sig;

    std::array<uint8_t, MD5_DIGEST_LENGTH> sigp;
    MD5(reinterpret_cast<const uint8_t*>(sig.c_str()), sig.length(), sigp.data());
    sig.clear();

    hex(sigp, std::back_inserter(sig));
    TRACE_LOG(logject) << "MD5 sig: " << sig;

    method.getParameters().front().addMember("api_sig", sig);

    return std::move(method);
}

std::string Service::postMethod(const Method& method) {
    return std::move(pimpl->postMethod(method));
}

void Service::playlistChangeSlot(std::shared_ptr<Melosic::Core::Playlist> playlist) {
//    if(playlist && *playlist)
//        trackChangedSlot(*playlist->currentTrack());
}

void Service::trackChangedSlot(const Melosic::Core::Track& newTrack) {
    pimpl->currentTrack_ = std::make_shared<Track>(shared_from_this(), newTrack);
}

std::shared_ptr<Track> Service::currentTrack() {
    return pimpl->currentTrack_;
}

User& Service::setUser(User u) {
    return pimpl->setUser(std::move(u));
}

User& Service::getUser() {
    return pimpl->getUser();
}

}
