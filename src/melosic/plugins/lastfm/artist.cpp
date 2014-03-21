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

#include <list>
#include <thread>
#include <mutex>
using std::unique_lock; using std::lock_guard;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
using namespace boost::property_tree::xml_parser;
#include <shared_mutex>
#include <boost/thread/shared_lock_guard.hpp>
using shared_mutex = std::shared_timed_mutex;
template <typename Mutex>
using shared_lock = boost::shared_lock_guard<Mutex>;

#include <melosic/common/thread.hpp>

#include "artist.hpp"
#include "service.hpp"
#include "tag.hpp"

namespace LastFM {

struct Artist::impl {
    impl(std::weak_ptr<Service> lastserv) : lastserv(lastserv) {}
    impl(std::weak_ptr<Service> lastserv, const std::string& name)
        : lastserv(lastserv),
          name(name)
    {}

private:
    bool getInfo_impl(std::shared_ptr<Service> lastserv, bool autocorrect) {
        if(!lastserv)
            return false;
        Method method = lastserv->prepareMethodCall("artist.getInfo");
        Parameter& p = method.addParameter()
                .addMember("artist", name)
                .addMember("api_key", lastserv->apiKey());
        if(autocorrect)
            p.addMember("autocorrect[1]");

        std::string reply(std::move(lastserv->postMethod(method)));
        if(reply.empty())
            return false;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            //TODO: handle error
            return false;
        }

        ptree = ptree.get_child("lfm.artist");
        lock_guard<Mutex> l(mu);
        if(autocorrect) {
            name = ptree.get<std::string>("name", name);
        }
        url = network::uri(ptree.get<std::string>("url", ""));
        tags.clear();
        for(const boost::property_tree::ptree::value_type& val : ptree.get_child("tags")) {
            tags.emplace_back(val.second.get<std::string>("name"), val.second.get<std::string>("url"));
        }
        biographySummary = ptree.get<std::string>("bio.summary", "");
        biography = ptree.get<std::string>("bio.content", "");

        return true;
    }

public:
    std::future<bool> getInfo(bool autocorrect) {
        unique_lock<Mutex> l(mu);
        std::shared_ptr<Service> lastserv = this->lastserv.lock();
        l.unlock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        return lastserv->getThreadManager()->enqueue(&impl::getInfo_impl, this, lastserv, autocorrect);
    }

public:
    const std::string& getName() {
        shared_lock<Mutex> l(mu);
        return name;
    }

    const network::uri& getUrl() {
        shared_lock<Mutex> l(mu);
        return url;
    }

    std::weak_ptr<Service> lastserv;
private:
    std::string name;
    network::uri url;
    std::string biographySummary, biography;
//    bool streamable;
    std::list<Tag> tags;

    typedef shared_mutex Mutex;
    Mutex mu;
};

Artist::Artist(std::weak_ptr<Service> lastserv) : pimpl(std::move(std::make_shared<impl>(lastserv))) {}

Artist::Artist(std::weak_ptr<Service> lastserv, const std::string& artist)
    : pimpl(std::move(std::make_shared<impl>(lastserv, artist)))
{}

Artist& Artist::operator=(Artist&& b){
    pimpl = std::move(b.pimpl);
    return *this;
}

Artist& Artist::operator=(const std::string& artist) {
    if(artist != getName()) {
        pimpl = std::move(std::make_shared<impl>(pimpl->lastserv, artist));
    }
    return *this;
}

Artist::operator bool() {
    return !getName().empty();
}

const std::string& Artist::getName() const {
    return pimpl->getName();
}

const network::uri& Artist::getUrl() const {
    return pimpl->getUrl();
}

std::future<bool> Artist::fetchInfo(bool autocorrect) {
    return pimpl->getInfo(autocorrect);
}

}//namespace LastFM
