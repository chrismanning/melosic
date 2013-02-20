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
using std::unique_lock; using std::lock_guard;
#include <functional>
namespace ph = std::placeholders;

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
using namespace boost::property_tree::xml_parser;
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
using boost::shared_mutex; using boost::shared_lock_guard;

#include <melosic/core/track.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/thread.hpp>

#include "track.hpp"
#include "service.hpp"
#include "tag.hpp"
#include "artist.hpp"
#include "utilities.hpp"

static Melosic::Logger::Logger logject(logging::keywords::channel = "LastFM::Track");

namespace LastFM {

struct Track::impl {
    impl(std::weak_ptr<Service> lastserv,
         const std::string& name,
         const std::string& artist,
         const std::string& url)
        : lastserv(lastserv),
          name(name),
          artist(lastserv, artist),
          url(url)
    {}

    impl(std::weak_ptr<Service> lastserv, const Melosic::Track& track)
        : lastserv(lastserv), artist(lastserv)
    {
        {   auto artist = track.getTag("artist");
            if(artist != "?")
                this->artist = Artist(lastserv, artist);}
        {   auto title = track.getTag("title");
            if(title != "?")
                this->name = title;}
//        {   auto album = track.getTag("album");
//            if(album != "?")
//                this->album = album;}
        {   auto tracknum = track.getTag("tracknumber");
            if(tracknum != "?")
                this->tracknumber = boost::lexical_cast<int>(tracknum);}
        this->duration = chrono::duration_cast<chrono::milliseconds>(track.duration()).count();
        timestamp = chrono::system_clock::to_time_t(chrono::system_clock::now());
    }

private:
    bool getInfo_impl(std::shared_ptr<Service> lastserv, bool autocorrect) {
        if(!lastserv)
            return false;
        TRACE_LOG(logject) << "In getInfo";
        if(!artist) {
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
                .addMember("artist", artist.getName())
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

        ptree = ptree.get_child("lfm.track");
        if(autocorrect) {
            lock_guard<Mutex> l(mu);
            name = ptree.get<std::string>("name");
            artist = Artist(this->lastserv, ptree.get<std::string>("artist.name"));
        }
        unique_lock<Mutex> l(mu);
        url = network::uri(ptree.get<std::string>("url"));
        topTags_.clear();
        for(const boost::property_tree::ptree::value_type& val : ptree.get_child("toptags")) {
            topTags_.emplace_back(val.second.get<std::string>("name"), val.second.get<std::string>("url"));
        }
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

private:
    bool scrobble_impl(std::shared_ptr<Service> lastserv) {
        if(!lastserv)
            return false;
        TRACE_LOG(logject) << "In scrobble";
        Method method = lastserv->prepareMethodCall("track.scrobble");
        method.addParameter()
                .addMember("artist", artist.getName())
                .addMember("track", name)
                .addMember("api_key", lastserv->apiKey())
                .addMember("timestamp", boost::lexical_cast<std::string>(timestamp));

        std::string reply(std::move(lastserv->postMethod(lastserv->sign(std::move(method)))));
        if(reply.empty())
            return false;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            //TODO: handle error
            return false;
        }

        return true;
    }

public:
    std::future<bool> scrobble() {
        unique_lock<Mutex> l(mu);
        std::shared_ptr<Service> lastserv = this->lastserv.lock();
        l.unlock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        return lastserv->getThreadManager()->enqueue(&impl::scrobble_impl, this, lastserv);
    }

private:
    bool updateNowPlaying_impl(std::shared_ptr<Service> lastserv) {
        if(!lastserv)
            return false;
        TRACE_LOG(logject) << "In updateNowPlaying";
        Method method = lastserv->prepareMethodCall("track.updateNowPlaying");
        method.addParameter()
                .addMember("artist", artist.getName())
                .addMember("track", name)
                .addMember("api_key", lastserv->apiKey());

        std::string reply(std::move(lastserv->postMethod(lastserv->sign(std::move(method)))));
        if(reply.empty())
            return false;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            //TODO: handle error
            return false;
        }

        return true;
    }

public:
    std::future<bool> updateNowPlaying() {
        unique_lock<Mutex> l(mu);
        std::shared_ptr<Service> lastserv = this->lastserv.lock();
        l.unlock();
        if(!lastserv) {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
        return lastserv->getThreadManager()->enqueue(&impl::updateNowPlaying_impl, this, lastserv);
    }

    ForwardRange<Tag> topTags() {
        shared_lock_guard<Mutex> l(mu);
        return topTags_;
    }

    const std::string& getName() {
        shared_lock_guard<Mutex> l(mu);
        return name;
    }

    const Artist& getArtist() {
        shared_lock_guard<Mutex> l(mu);
        return artist;
    }

    const network::uri& getUrl() {
        shared_lock_guard<Mutex> l(mu);
        return url;
    }

    const std::string& getWiki() {
        shared_lock_guard<Mutex> l(mu);
        return wiki;
    }

private:
    std::weak_ptr<Service> lastserv;
    std::string name;
    Artist artist;
    network::uri url;
    std::list<Tag> topTags_;
    int tracknumber = 0;
    long duration = 0;
    uint64_t timestamp = 0;
//    Album album;
    std::string wiki;
    std::list<Track> similar;
    typedef shared_mutex Mutex;
    Mutex mu;
};

Track::Track(std::weak_ptr<Service> lastserv,
             const std::string& name,
             const std::string& artist,
             const std::string& url)
    : pimpl(new impl(std::move(lastserv), name, artist, url)) {}
Track::Track(std::weak_ptr<Service> lastserv, const Melosic::Track& track)
    : pimpl(new impl(lastserv, track)) {}

Track::~Track() {}

std::future<bool> Track::fetchInfo(bool autocorrect) {
    return pimpl->getInfo(autocorrect);
}

std::future<bool> Track::scrobble() {
    return pimpl->scrobble();
}

std::future<bool> Track::updateNowPlaying() {
    return pimpl->updateNowPlaying();
}

void print(const boost::property_tree::ptree& pt) {
    using boost::property_tree::ptree;
    for(const ptree::value_type& val : pt) {
        std::clog << val.first << ": " << val.second.get_value<std::string>() << std::endl;
        print(val.second);
    }
}

ForwardRange<Tag> Track::topTags() const {
    return pimpl->topTags();
}

const std::string& Track::getName() const {
    return pimpl->getName();
}

const Artist& Track::getArtist() const {
    return pimpl->getArtist();
}

const network::uri& Track::getUrl() const {
    return pimpl->getUrl();
}

const std::string& Track::getWiki() const {
    return pimpl->getWiki();
}

}
