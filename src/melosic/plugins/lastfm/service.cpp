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

#include <network/uri.hpp>
#include <network/http/client.hpp>
#include <network/http/request.hpp>
#include <network/http/response.hpp>
#include <network/uri/uri_builder.hpp>

#include <melosic/melin/logging.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/common/error.hpp>

#include "service.hpp"
#include "track.hpp"

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
        network::uri_builder url;
        const Parameter& param = method.getParameters().front();

        strstr << "method=" << method.methodName;
        for(const auto& member : param.getMembers()) {
            strstr << "&" << member.first << "=" << member.second;
        }
        std::string qstr(strstr.str());
        std::replace(qstr.begin(), qstr.end(), ' ', '+');
        url.scheme("http")
           .host("ws.audioscrobbler.com")
           .path("/2.0/")
           .query(qstr);
        TRACE_LOG(logject) << "Query uri: " << url.uri();
        request.set_uri(url.uri());

        network::http::response response = client.post(request);

        std::string str;
        uint16_t status = 0;
        try {
            response.get_status(status);
            TRACE_LOG(logject) << "HTTP response status: " << status;
            if(status != 200) {
                response.get_status_message(str);
                ERROR_LOG(logject) << str;
                BOOST_THROW_EXCEPTION(Melosic::HttpException() << Melosic::ErrorTag::HttpStatus({status, str}));
            }

            response.get_body(str);
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
        catch(std::runtime_error& e) {
            ERROR_LOG(logject) << "Network: " << e.what();
        }

        request.set_uri("");

        return str;
    }

private:
    network::http::client client;
    network::http::request request;
    const std::string apiKey;
    const std::string sharedSecret;
    std::shared_ptr<Track> currentTrack_;
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

std::string Service::postMethod(const Method& method) {
    return std::move(pimpl->postMethod(method));
}

void Service::playlistChangeSlot(std::shared_ptr<Melosic::Playlist> playlist) {
//    playlist->connectTrackChangedSlot(
//                Melosic::Playlist::TrackChangedSignal::slot_type(&Service::trackChangedSlot,
//                                                                 shared_from_this(),
//                                                                 _1, _2));
    trackChangedSlot(*playlist->currentTrack(), true);
}

void Service::trackChangedSlot(const Melosic::Track& newTrack, bool alive) {
    if(!alive) {
        return;
    }
    pimpl->currentTrack_ = std::make_shared<Track>(shared_from_this(), newTrack);
}

std::shared_ptr<Track> Service::currentTrack() {
    return pimpl->currentTrack_;
}

}
