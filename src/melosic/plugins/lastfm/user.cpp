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

#include <string>
#include <thread>
#include <mutex>
using std::mutex;
using std::unique_lock;
using std::lock_guard;
#include <functional>
namespace ph = std::placeholders;
#include <future>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
using namespace boost::property_tree::xml_parser;
#include <boost/lexical_cast.hpp>

#include <network/uri.hpp>

#include <melosic/melin/logging.hpp>
#include <melosic/executors/default_executor.hpp>
namespace executors = Melosic::executors;

#include "user.hpp"
#include "service.hpp"

namespace lastfm {

struct user::impl : std::enable_shared_from_this<impl> {
    impl(std::weak_ptr<service> lastserv) : impl(lastserv, "", "") {
    }
    impl(std::weak_ptr<service> lastserv, const std::string& username) : impl(lastserv, username, "") {
    }
    impl(std::weak_ptr<service> lastserv, const std::string& username, const std::string& sessionKey)
        : lastserv(lastserv), username(username), sessionKey(sessionKey),
          logject(logging::keywords::channel = "lastfm::user") {
    }

  private:
    bool getInfo_impl(const std::shared_ptr<service>& lastserv) {
        if(!lastserv)
            return false;
        TRACE_LOG(logject) << "In getInfo";
        Method method = lastserv->prepareMethodCall("user.getInfo");
        method.addParameter().addMember("user", username).addMember("api_key", lastserv->api_key());
        std::string reply(std::move(lastserv->postMethod(method)));
        if(reply.empty())
            return false;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            // TODO: handle error
            return false;
        }

        ptree = ptree.get_child("lfm.user");
        lock_guard<Mutex> l(mu);
        url = network::uri(ptree.get<std::string>("url"));
        gender = boost::lexical_cast<char>(ptree.get<std::string>("gender", ""));
        age = boost::lexical_cast<uint8_t>(ptree.get<std::string>("age", "0"));
        country = ptree.get<std::string>("country", "");
        registered = boost::lexical_cast<std::time_t>(ptree.get<std::string>("registered", "0"));

        return true;
    }

  public:
    std::future<bool> getInfo() {
        std::shared_ptr<service> lastserv = this->lastserv.lock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        std::packaged_task<bool()> task([ self = shared_from_this(), lastserv ]() {
            return self->getInfo_impl(lastserv);
        });
        auto fut = task.get_future();
        asio::post(std::move(task));

        return fut;
    }

  private:
    bool authenticate_impl(const std::shared_ptr<service>& lastserv) {
        return static_cast<bool>(lastserv);
    }

  public:
    std::future<bool> authenticate() {
        std::shared_ptr<service> lastserv = this->lastserv.lock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        std::packaged_task<bool()> task([ self = shared_from_this(), lastserv ]() {
            return self->authenticate_impl(lastserv);
        });
        auto fut = task.get_future();
        asio::post(std::move(task));

        return fut;
    }

    const std::string& getSessionKey() {
        lock_guard<Mutex> l(mu);
        return sessionKey;
    }
    void setSessionKey(const std::string& sk) {
        lock_guard<Mutex> l(mu);
        TRACE_LOG(logject) << "Set session key";
        sessionKey = sk;
    }

  private:
    std::weak_ptr<service> lastserv;
    std::string username;
    std::string sessionKey;
    Melosic::Logger::Logger logject;
    network::uri url;
    char gender = 0;
    uint8_t age = 0;
    std::string country;
    std::time_t registered;

    typedef mutex Mutex;
    Mutex mu;
};

user::user() : pimpl(nullptr) {
}

user::user(std::weak_ptr<service> lastserv, const std::string& username)
    : pimpl(std::make_shared<impl>(lastserv, username)) {
}

user::~user() {
}

std::future<bool> user::getInfo() {
    return pimpl->getInfo();
}

user::user(std::weak_ptr<service> lastserv, const std::string& username, const std::string& sessionKey)
    : pimpl(new impl(lastserv, username, sessionKey)) {
}

user::user(user&& b) : pimpl(std::move(b.pimpl)) {
}

user& user::operator=(user&& b) {
    pimpl = std::move(b.pimpl);
    return *this;
}

const std::string& user::getSessionKey() const {
    return pimpl->getSessionKey();
}

void user::setSessionKey(const std::string& sk) {
    pimpl->setSessionKey(sk);
}

user::operator bool() {
    return static_cast<bool>(pimpl);
}

} // namespace lastfm
