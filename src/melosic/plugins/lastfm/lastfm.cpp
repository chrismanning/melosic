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

#include <melosic/melin/exports.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/melin/sigslots/connection.hpp>
#include <melosic/melin/sigslots/slots.hpp>
using namespace Melosic;

#include "lastfm.hpp"
#include "lastfmconfig.hpp"
#include "scrobbler.hpp"
#include "service.hpp"
using namespace LastFM;

static Logger::Logger logject(boost::log::keywords::channel = "LastFM");
static LastFmConfig conf;

static constexpr Plugin::Info lastFmInfo("LastFM",
                                         Plugin::Type::utility | Plugin::Type::service | Plugin::Type::gui,
                                         Plugin::Version(1,0,0)
                                        );

static std::shared_ptr<Service> lastserv;
static std::shared_ptr<Scrobbler> scrobbler;
static Slots::Manager* slotman = nullptr;
static std::list<Signals::ScopedConnection> scrobConnections;

void refreshConfig(const std::string& key, const Config::VarType& value) {
    if(key == "username") {
//        lastfm::ws::Username = QString::fromStdString(boost::get<std::string>(value));
    }
    else if(key == "enable scrobbling") {
        if(boost::get<bool>(value)) {
            scrobConnections.clear();
            scrobbler.reset(new Scrobbler(lastserv));
            if(slotman == nullptr)
                return;

//            scrobConnections.emplace_back(slotman->get<>());
//                        kernel->getPlayer().connectNotifySlot(
//                            Player::NotifySignal::slot_type(&Scrobbler::notifySlot,
//                                                            scrobbler.get(),
//                                                            _1,
//                                                            _2).track_foreign(scrobbler)));
//            scrobConnections.emplace_back(
//                        kernel->getPlayer().connectStateSlot(
//                            Player::StateSignal::slot_type(&Scrobbler::stateChangedSlot,
//                                                           scrobbler.get(),
//                                                           _1).track_foreign(scrobbler)));
//            scrobConnections.emplace_back(
//                        kernel->getPlayer().connectPlaylistChangeSlot(
//                            Player::PlaylistChangeSignal::slot_type(&Scrobbler::playlistChangeSlot,
//                                                                    scrobbler.get(),
//                                                                    _1).track_foreign(scrobbler)));
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

static Signals::Connection varConnection;

extern "C" void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::lastFmInfo;
    funs << registerConfig << registerSlots;
}

extern "C" void registerConfig(Config::Manager* confman) {
    ::conf.putNode("username", std::string(""));
    ::conf.putNode("session key", std::string(""));
    ::conf.putNode("enable scrobbling", false);
    auto& c = confman->getConfigRoot();
    if(!c.existsChild("lastfm")) {
        varConnection = c.putChild("lastfm", ::conf)
                .get<Signals::Config::VariableUpdate>()
                    .connect(refreshConfig);
    }
    else {
        varConnection = c.getChild("lastfm")
                .get<Signals::Config::VariableUpdate>()
                    .connect(refreshConfig);
    }

    lastserv.reset(new Service("47ee6adfdb3c68daeea2786add5e242d",
                               "64a3811653376876431daad679ce5b67"));
}

extern "C" void registerSlots(Slots::Manager* slotman) {
    ::slotman = slotman;
}

//    refreshConfig("username", std::string("fat_chris"));
//    refreshConfig("enable scrobbling", true);
//    refreshConfig("session key", std::string("5249ca2b30f7f227910fd4b5bdfe8785"));

extern "C" void destroyPlugin() {}
