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
using std::unique_lock;
using std::lock_guard;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
using namespace boost::property_tree::xml_parser;
#include <shared_mutex>
#include <boost/thread/shared_lock_guard.hpp>
using shared_mutex = std::shared_timed_mutex;
template <typename Mutex> using shared_lock = boost::shared_lock_guard<Mutex>;

#include <melosic/executors/default_executor.hpp>
namespace executors = Melosic::executors;

#include "artist.hpp"
#include "service.hpp"
#include "tag.hpp"

namespace lastfm {

struct artist::impl : std::enable_shared_from_this<impl> {
    impl(std::weak_ptr<service> lastserv) : lastserv(lastserv) {}
    impl(std::weak_ptr<service> lastserv, const std::string& name) : lastserv(lastserv), name(name) {}

  private:
    bool getInfo_impl(const std::shared_ptr<service>& lastserv, bool autocorrect) {
        if(!lastserv)
            return false;
        Method method = lastserv->prepareMethodCall("artist.getInfo");
        Parameter& p = method.addParameter().addMember("artist", name).addMember("api_key", lastserv->api_key());
        if(autocorrect)
            p.addMember("autocorrect[1]");

        auto reply = lastserv->postMethod(method);
        if(reply.empty())
            return false;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            // TODO: handle error
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
            tags.emplace_back(val.second.get<std::string>("name"), network::uri{val.second.get<std::string>("url")});
        }
        biographySummary = ptree.get<std::string>("bio.summary", "");
        biography = ptree.get<std::string>("bio.content", "");

        return true;
    }

  public:
    std::future<bool> getInfo(bool autocorrect) {
        unique_lock<Mutex> l(mu);
        std::shared_ptr<service> lastserv = this->lastserv.lock();
        l.unlock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        std::packaged_task<bool()> task([ self = shared_from_this(), lastserv, autocorrect ]() {
            return self->getInfo_impl(lastserv, autocorrect);
        });
        auto fut = task.get_future();
        asio::post(std::move(task));

        return fut;
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

    std::weak_ptr<service> lastserv;

  private:
    std::string name;
    network::uri url;
    std::string biographySummary, biography;
    //    bool streamable;
    std::vector<tag> tags;

    typedef shared_mutex Mutex;
    Mutex mu;
};

artist::artist(std::weak_ptr<service> lastserv) : pimpl(std::move(std::make_shared<impl>(lastserv))) {}

artist::artist(std::weak_ptr<service> lastserv, const std::string& artist)
    : pimpl(std::move(std::make_shared<impl>(lastserv, artist))) {}

artist& artist::operator=(artist&& b) {
    pimpl = std::move(b.pimpl);
    return *this;
}

artist& artist::operator=(const std::string& artist) {
    if(artist != getName()) {
        pimpl = std::move(std::make_shared<impl>(pimpl->lastserv, artist));
    }
    return *this;
}

artist::operator bool() { return !getName().empty(); }

const std::string& artist::getName() const { return pimpl->getName(); }

const network::uri& artist::getUrl() const { return pimpl->getUrl(); }

std::future<bool> artist::fetchInfo(bool autocorrect) { return pimpl->getInfo(autocorrect); }

} // namespace lastfm
