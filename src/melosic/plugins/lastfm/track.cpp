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

#include <thread>
#include <mutex>
using std::unique_lock;
using std::lock_guard;
#include <functional>
namespace ph = std::placeholders;
#include <string>
using namespace std::literals;

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
using namespace boost::property_tree::xml_parser;
#include <shared_mutex>
#include <boost/thread/shared_lock_guard.hpp>
using shared_mutex = std::shared_timed_mutex;
template <typename Mutex> using shared_lock = boost::shared_lock_guard<Mutex>;

#include <melosic/core/track.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/executors/default_executor.hpp>
namespace executors = Melosic::executors;

#include "track.hpp"
#include "service.hpp"
#include "tag.hpp"
#include "artist.hpp"
#include "utilities.hpp"

static Melosic::Logger::Logger logject(logging::keywords::channel = "lastfm::track");

namespace lastfm {

struct track::impl : std::enable_shared_from_this<impl> {
    impl(std::weak_ptr<service> lastserv, const std::string& name, const std::string& artist, const std::string& url)
        : lastserv(lastserv), name(name), m_artist(lastserv, artist), url(url) {
    }

    impl(std::weak_ptr<service> lastserv, const Melosic::Core::Track& track) : lastserv(lastserv), m_artist(lastserv) {
        {
            auto artist_name = ""s; // track.getTag("artist");
            if(artist_name != "?")
                this->m_artist = artist(lastserv, artist_name);
        }
        {
            auto title = ""s; // track.getTag("title");
            if(title != "?")
                this->name = title;
        }
        //        {   auto album = track.getTag("album");
        //            if(album != "?")
        //                this->album = album;}
        {
            auto tracknum = ""s; // track.getTag("tracknumber");
            if(tracknum != "?")
                this->tracknumber = boost::lexical_cast<int>(tracknum);
        }
        this->duration = track.duration().count();
        timestamp = chrono::system_clock::to_time_t(chrono::system_clock::now());
    }

  private:
    bool getInfo_impl(std::shared_ptr<service> lastserv, bool autocorrect) {
        if(!lastserv)
            return false;
        TRACE_LOG(logject) << "In getInfo";
        if(!m_artist) {
            TRACE_LOG(logject) << "getInfo: Required parameter \"artist\" not set. Returning.";
            return false;
        }
        if(name.empty()) {
            TRACE_LOG(logject) << "getInfo: Required parameter \"name\" not set. Returning.";
            return false;
        }
        Method method = lastserv->prepareMethodCall("track.getInfo");
        Parameter& p = method.addParameter()
                           .addMember("track", name)
                           .addMember("artist", m_artist.getName())
                           .addMember("api_key", lastserv->api_key());
        if(autocorrect)
            p.addMember("autocorrect[1]");
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

        ptree = ptree.get_child("lfm.track");
        if(autocorrect) {
            lock_guard<Mutex> l(mu);
            name = ptree.get<std::string>("name");
            m_artist = artist(this->lastserv, ptree.get<std::string>("artist.name"));
        }
        unique_lock<Mutex> l(mu);
        url = network::uri(ptree.get<std::string>("url"));
        topTags_.clear();
        for(const boost::property_tree::ptree::value_type& val : ptree.get_child("toptags")) {
            topTags_.emplace_back(val.second.get<std::string>("name"),
                                  network::uri{val.second.get<std::string>("url")});
        }
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

  private:
    bool scrobble_impl(std::shared_ptr<service> lastserv) {
        if(!lastserv)
            return false;
        TRACE_LOG(logject) << "In scrobble";
        Method method = lastserv->prepareMethodCall("track.scrobble");
        method.addParameter()
            .addMember("artist", m_artist.getName())
            .addMember("track", name)
            .addMember("api_key", lastserv->api_key())
            .addMember("timestamp", boost::lexical_cast<std::string>(timestamp));

        std::string reply(std::move(lastserv->postMethod(lastserv->sign(std::move(method)))));
        if(reply.empty())
            return false;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            // TODO: handle error
            return false;
        }

        return true;
    }

  public:
    std::future<bool> scrobble() {
        unique_lock<Mutex> l(mu);
        std::shared_ptr<service> lastserv = this->lastserv.lock();
        l.unlock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        std::packaged_task<bool()> task([ self = shared_from_this(), lastserv ]() {
            return self->scrobble_impl(lastserv);
        });
        auto fut = task.get_future();
        asio::post(std::move(task));

        return fut;
    }

  private:
    bool updateNowPlaying_impl(std::shared_ptr<service> lastserv) {
        if(!lastserv)
            return false;
        TRACE_LOG(logject) << "In updateNowPlaying";
        Method method = lastserv->prepareMethodCall("track.updateNowPlaying");
        method.addParameter()
            .addMember("artist", m_artist.getName())
            .addMember("track", name)
            .addMember("api_key", lastserv->api_key());

        std::string reply(std::move(lastserv->postMethod(lastserv->sign(std::move(method)))));
        if(reply.empty())
            return false;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            // TODO: handle error
            return false;
        }

        return true;
    }

  public:
    std::future<bool> updateNowPlaying() {
        unique_lock<Mutex> l(mu);
        std::shared_ptr<service> lastserv = this->lastserv.lock();
        l.unlock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        std::packaged_task<bool()> task([ self = shared_from_this(), lastserv ]() {
            return self->updateNowPlaying_impl(lastserv);
        });
        auto fut = task.get_future();
        asio::post(std::move(task));

        return fut;
    }

    ForwardRange<tag> topTags() {
        shared_lock<Mutex> l(mu);
        return topTags_;
    }

    const std::string& getName() {
        shared_lock<Mutex> l(mu);
        return name;
    }

    const artist& getArtist() {
        shared_lock<Mutex> l(mu);
        return m_artist;
    }

    const network::uri& getUrl() {
        shared_lock<Mutex> l(mu);
        return url;
    }

    const std::string& getWiki() {
        shared_lock<Mutex> l(mu);
        return wiki;
    }

  private:
    std::weak_ptr<service> lastserv;
    std::string name;
    artist m_artist;
    network::uri url;
    std::list<tag> topTags_;
    int tracknumber = 0;
    long duration = 0;
    uint64_t timestamp = 0;
    //    Album album;
    std::string wiki;
    std::list<track> similar;
    typedef shared_mutex Mutex;
    Mutex mu;
};

track::track(std::weak_ptr<service> lastserv, const std::string& name, const std::string& artist,
             const std::string& url)
    : pimpl(new impl(std::move(lastserv), name, artist, url)) {
}
track::track(std::weak_ptr<service> lastserv, const Melosic::Core::Track& track)
    : pimpl(std::make_shared<impl>(lastserv, track)) {
}

track::~track() {
}

std::future<bool> track::fetchInfo(bool autocorrect) {
    return pimpl->getInfo(autocorrect);
}

std::future<bool> track::scrobble() {
    return pimpl->scrobble();
}

std::future<bool> track::updateNowPlaying() {
    return pimpl->updateNowPlaying();
}

void print(const boost::property_tree::ptree& pt) {
    using boost::property_tree::ptree;
    for(const ptree::value_type& val : pt) {
        std::clog << val.first << ": " << val.second.get_value<std::string>() << std::endl;
        print(val.second);
    }
}

ForwardRange<tag> track::topTags() const {
    return pimpl->topTags();
}

const std::string& track::getName() const {
    return pimpl->getName();
}

const artist& track::getArtist() const {
    return pimpl->getArtist();
}

const network::uri& track::getUrl() const {
    return pimpl->getUrl();
}

const std::string& track::getWiki() const {
    return pimpl->getWiki();
}
}
