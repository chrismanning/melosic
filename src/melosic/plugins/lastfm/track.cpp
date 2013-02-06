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

#include "track.hpp"
#include "service.hpp"
#include "tag.hpp"
#include "artist.hpp"
#include "utilities.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
using namespace boost::property_tree::xml_parser;
#include <boost/range/adaptor/filtered.hpp>
using boost::adaptors::filtered;
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
using boost::shared_mutex; using boost::shared_lock_guard;
#include <thread>
using std::thread; using std::unique_lock; using std::lock_guard;

#include <melosic/core/track.hpp>
#include <melosic/melin/logging.hpp>

#include <opqit/opaque_iterator.hpp>

static Melosic::Logger::Logger logject(boost::log::keywords::channel = "LastFM::Track");

namespace LastFM {

struct Track::impl {
    impl(std::weak_ptr<Service> lastserv,
         const std::string& name,
         const std::string& artist,
         const std::string& url)
        : name(name),
          artist(artist),
          url(url),
          lastserv(lastserv)
    {}

    impl(std::weak_ptr<Service> lastserv, const Melosic::Track& track)
        : lastserv(lastserv)
    {
        {   auto artist = track.getTag("artist");
            if(artist != "?")
                this->artist = artist;}
        {   auto title = track.getTag("title");
            if(title != "?")
                this->name = title;}
    //    {   auto album = track.getTag("album");
    //        if(album != "?")
    //            this->album = album;}
        {   auto tracknum = track.getTag("tracknumber");
            if(tracknum != "?")
                this->tracknumber = boost::lexical_cast<int>(tracknum);}
        this->duration = chrono::duration_cast<chrono::milliseconds>(track.duration()).count();
    }

    void getInfo_impl(std::function<void()> finished, bool autocorrect) {
        TRACE_LOG(logject) << "In getInfo";
        std::shared_ptr<Service> lastserv = this->lastserv.lock();
        if(!lastserv)
            return;
        Method method = lastserv->prepareMethodCall("track.getInfo");
        Parameter& p = method.addParameter()
                .addMember("track", name)
                .addMember("artist", artist.name)
                .addMember("api_key", lastserv->apiKey());
        if(autocorrect)
            p.addMember("autocorrect[1]");
        std::string reply;
        reply = lastserv->postMethod(method);
        if(reply.empty())
            return;

        boost::property_tree::ptree ptree;
        std::stringstream ss(reply);
        boost::property_tree::xml_parser::read_xml(ss, ptree, trim_whitespace);

        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
            //TODO: handle error
            return;
        }

        ptree = ptree.get_child("lfm.track");
        if(autocorrect) {
            lock_guard<Mutex> m(mutex);
            name = ptree.get<std::string>("name");
            artist = Artist(ptree.get<std::string>("artist.name"), ptree.get<std::string>("artist.url"));
        }
        unique_lock<Mutex> m(mutex);
        url = network::uri(ptree.get<std::string>("url"));
        topTags.clear();
        for(const boost::property_tree::ptree::value_type& val : ptree.get_child("toptags")) {
            topTags.emplace_back(val.second.get<std::string>("name"), val.second.get<std::string>("url"));
        }
        m.unlock();
        finished();
    }
    void getInfo(std::function<void()> finished, bool autocorrect) {
        threads.emplace_back(std::bind(&impl::getInfo_impl, this, finished, autocorrect));
    }

//    boost::iterator_range<opqit::opaque_iterator<Track, opqit::forward> >
//    getSimilar(std::shared_ptr<Service> lastserv, unsigned limit) {
//        if(!similar.empty())
//            return boost::make_iterator_range(similar);

//        Method method = lastserv->prepareMethodCall("track.getsimilar");
//        method.addParameter()
//                .addMember("track", name)
//                .addMember("artist", artist.name)
//                .addMember("api_key", lastserv->apiKey())
//                .addMember("limit", boost::lexical_cast<std::string>(limit));
//        std::string reply;
//        reply = lastserv->postMethod(method);
//        if(reply.empty())
//            return boost::make_iterator_range(similar);

//        boost::property_tree::ptree ptree;
//        std::stringstream ss(reply);
//        boost::property_tree::xml_parser::read_xml(ss, ptree, trim_whitespace | no_comments);

//        if(ptree.get<std::string>("lfm.<xmlattr>.status", "failed") != "ok") {
//            //TODO: handle error
//            return boost::make_iterator_range(similar);
//        }

//        ptree = ptree.get_child("lfm.similartracks");
//        similar.clear();
//        for(const boost::property_tree::ptree::value_type& val : ptree | filtered(NoAttributes())) {
//            similar.emplace_back(val.second.get<std::string>("name"),
//                                 val.second.get<std::string>("artist.name"),
//                                 val.second.get<std::string>("url"));
//        }

//        return boost::make_iterator_range(similar);
//    }

    const std::string& getName() {
        shared_lock_guard<Mutex> m(mutex);
        return name;
    }

    const Artist& getArtist() {
        shared_lock_guard<Mutex> m(mutex);
        return artist;
    }

    const network::uri& getUrl() {
        shared_lock_guard<Mutex> m(mutex);
        return url;
    }

    const std::string& getWiki() {
        shared_lock_guard<Mutex> m(mutex);
        return wiki;
    }

private:
    std::string name;
    Artist artist;
    network::uri url;
    std::weak_ptr<Service> lastserv;
    std::list<Tag> topTags;
    int tracknumber = 0;
    long duration = 0;
    uint64_t timestamp = 0;
    //Album album;
    std::string wiki;
    std::list<Track> similar;
    typedef shared_mutex Mutex;
    Mutex mutex;
    std::list<thread> threads;
};

Track::Track(std::shared_ptr<Service> lastserv,
             const std::string& name,
             const std::string& artist,
             const std::string& url)
    : pimpl(new impl(std::move(lastserv), name, artist, url)) {}
Track::Track(std::shared_ptr<Service> lastserv, const Melosic::Track& track)
    : pimpl(new impl(std::move(lastserv), track)) {}

void Track::getInfo(std::function<void(Track&)> callback, bool autocorrect) {
    pimpl->getInfo(std::bind(callback, *this), autocorrect);
}

void print(const boost::property_tree::ptree& pt) {
    using boost::property_tree::ptree;
    for(const ptree::value_type& val : pt) {
        std::clog << val.first << ": " << val.second.get_value<std::string>() << std::endl;
        print(val.second);
    }
}

//void Track::getInfo(bool autocorrect) {
//    pimpl->getInfo(autocorrect);
//}

//boost::iterator_range<opqit::opaque_iterator<Track, opqit::forward> >
//Track::getSimilar(std::shared_ptr<Service> lastserv, unsigned limit) {
//    return pimpl->getSimilar(lastserv, limit);
//}

template< typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>&
operator<<(std::basic_ostream<CharT, TraitsT>& out, const LastFM::Track& track) {
    return out << track.pimpl->getArtist().name << " - " << track.pimpl->getName() << " : " << track.pimpl->getUrl();
}

void Track::scrobble() {
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
