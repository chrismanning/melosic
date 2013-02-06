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

#include <boost/signals2/connection.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <network/uri.hpp>
#include <network/http/client.hpp>
#include <network/http/request.hpp>
#include <network/http/response.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/melin/logging.hpp>
using namespace Melosic;

#include "lastfm.hpp"
#include "lastfmconfig.hpp"
//BOOST_SERIALIZATION_FACTORY_0(Melosic::LastFmConfig)
//BOOST_CLASS_EXPORT_IMPLEMENT(Melosic::LastFmConfig)
#include "scrobbler.hpp"
#include "service.hpp"
using namespace LastFM;

static Logger::Logger logject(boost::log::keywords::channel = "LastFM");
static LastFmConfig conf;

static constexpr Plugin::Info lastFmInfo("LastFM",
                                     Plugin::Type::utility | Plugin::Type::service | Plugin::Type::gui,
                                     {1,0,0}
                                    );

static std::shared_ptr<Service> lastserv;
static std::shared_ptr<Scrobbler> scrobbler;
static std::list<boost::signals2::scoped_connection> scrobConnections;
static std::shared_ptr<Kernel> kernel;

void refreshConfig(const std::string& key, const Config::Base::VarType& value) {
    if(key == "username") {
//        lastfm::ws::Username = QString::fromStdString(boost::get<std::string>(value));
    }
    else if(key == "enable scrobbling") {
        if(boost::get<bool>(value)) {
            scrobbler.reset(new Scrobbler(lastserv));

            scrobConnections.clear();

            scrobConnections.emplace_back(
                        kernel->getPlayer().connectNotifySlot(
                            Player::NotifySignal::slot_type(&Scrobbler::notifySlot,
                                                            scrobbler.get(),
                                                            _1,
                                                            _2).track_foreign(scrobbler)));
            scrobConnections.emplace_back(
                        kernel->getPlayer().connectStateSlot(
                            Player::StateSignal::slot_type(&Scrobbler::stateChangedSlot,
                                                           scrobbler.get(),
                                                           _1).track_foreign(scrobbler)));
            scrobConnections.emplace_back(
                        kernel->getPlayer().connectPlaylistChangeSlot(
                            Player::PlaylistChangeSignal::slot_type(&Scrobbler::playlistChangeSlot,
                                                                    scrobbler.get(),
                                                                    _1).track_foreign(scrobbler)));
        }
        else {
            scrobConnections.clear();
            scrobbler.reset();
        }
    }
    else if(key == "session key") {
//        lastfm::ws::SessionKey = QString::fromStdString(boost::get<std::string>(value));
    }
    TRACE_LOG(logject) << "Updated variable: " << key;
}

static boost::signals2::scoped_connection varConnection;

extern "C" void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::lastFmInfo;
    funs << registerConfig;
}

extern "C" void registerConfig(Config::Manager* confman) {
    ::conf.putNode("username", std::string(""));
    ::conf.putNode("session key", std::string(""));
    ::conf.putNode("enable scrobbling", false);
    confman->getConfigRoot().putChild("lastfm", ::conf);
}

//    lastserv.reset(new Service("47ee6adfdb3c68daeea2786add5e242d",
//                               "64a3811653376876431daad679ce5b67"));
////    lastfm::ws::ApiKey = "47ee6adfdb3c68daeea2786add5e242d";
////    lastfm::ws::SharedSecret = "64a3811653376876431daad679ce5b67";

//    ::kernel = kernel->shared_from_this();
//    ::conf.putNode("username", std::string(""));
//    ::conf.putNode("session key", std::string(""));
//    ::conf.putNode("enable scrobbling", false);
//    auto& c = kernel->getConfig();
//    std::function<void(const std::string&,const Configuration::VarType&)> slot = refreshConfig;
//    if(!c.existsChild("lastfm")) {
//        varConnection = c.putChild("lastfm", ::conf).connectVariableUpdateSlot(slot);
//    }
//    else {
//        varConnection = c.getChild("lastfm").connectVariableUpdateSlot(slot);
//    }
//    refreshConfig("username", std::string("fat_chris"));
//    refreshConfig("enable scrobbling", true);
//    refreshConfig("session key", std::string("5249ca2b30f7f227910fd4b5bdfe8785"));
//}

extern "C" void destroyPlugin() {}
