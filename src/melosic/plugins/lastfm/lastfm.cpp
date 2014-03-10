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

#include <boost/variant.hpp>
#include <boost/config.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/common/connection.hpp>
#include <melosic/common/thread.hpp>
using namespace Melosic;

#include "lastfm.hpp"
#include "scrobbler.hpp"
#include "service.hpp"
#include "user.hpp"
using namespace LastFM;

static Logger::Logger logject(logging::keywords::channel = "LastFM");
Config::Conf conf{"LastFM"};

static constexpr Plugin::Info lastFmInfo("LastFM",
                                         Plugin::Type::utility | Plugin::Type::service | Plugin::Type::gui,
                                         Plugin::Version(1,0,0)
                                        );

static std::shared_ptr<Service> lastserv;
static std::shared_ptr<Scrobbler> scrobbler;
static Config::Manager* confman = nullptr;
static Thread::Manager* tman = nullptr;
static std::string sk;

void refreshConfig(const std::string& key, const Config::VarType& value) {
    try {
        if(key == "username") {
            if(lastserv->getUser() && !lastserv->getUser().getSessionKey().empty())
                sk = lastserv->getUser().getSessionKey();
            lastserv->setUser(User(lastserv, boost::get<std::string>(value), sk));
        }
        else if(key == "enable scrobbling") {
            if(boost::get<bool>(value)) {
//                if(slotman == nullptr)
//                    return;
                scrobbler.reset(new Scrobbler(lastserv));
            }
            else {
                scrobbler.reset();
            }
        }
        else if(key == "session key") {
            if(lastserv->getUser())
                lastserv->getUser().setSessionKey(boost::get<std::string>(value));
            else
                sk = boost::get<std::string>(value);
        }
        else
            assert(false);
        TRACE_LOG(logject) << "Config: Updated variable: " << key;
    }
    catch(boost::bad_get&) {
        ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
    }
}

static Signals::ScopedConnection varConnection;

extern "C" BOOST_SYMBOL_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::lastFmInfo;
    funs << registerConfig << registerTasks;

    lastserv.reset(new Service("47ee6adfdb3c68daeea2786add5e242d",
                               "64a3811653376876431daad679ce5b67",
                               tman));
}

extern "C" BOOST_SYMBOL_EXPORT void registerConfig(Config::Manager* confman) {
    ::confman = confman;

    ::conf.putNode("username", std::string(""));
    ::conf.putNode("session key", std::string(""));
    ::conf.putNode("enable scrobbling", false);
}

extern "C" BOOST_SYMBOL_EXPORT void registerTasks(Melosic::Thread::Manager* tman) {
    ::tman = tman;
}

//    refreshConfig("session key", std::string("5249ca2b30f7f227910fd4b5bdfe8785"));

extern "C" BOOST_SYMBOL_EXPORT void destroyPlugin() {}
