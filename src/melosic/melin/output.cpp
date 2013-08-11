/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#include <map>
#include <thread>
#include <mutex>
using std::mutex;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <string>

#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;
#include <boost/variant.hpp>

#include <melosic/common/error.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/common/signal_core.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/config.hpp>

#include "output.hpp"

namespace Melosic {
namespace Output {

struct PlayerSinkChanged : Signals::Signal<Signals::Output::PlayerSinkChanged> {
    using Super::Signal;
};

class Manager::impl {
public:
    impl(Config::Manager& confman) {
        conf.putNode("output device", "iec958:CARD=PCH,DEV=0"s);
        confman.getLoadedSignal().connect([this](Config::Conf& base) {
            TRACE_LOG(logject) << "Output conf loaded";
            auto& c = base.existsChild("Output")
                    ? base.getChild("Output")
                    : base.putChild("Output", conf);
            c.merge(conf);
            c.addDefaultFunc([=]() -> Config::Conf { return conf; });
            auto& varUpdate = c.getVariableUpdatedSignal();
            auto fun = [this](const std::string& key, const Config::VarType& val) {
                TRACE_LOG(logject) << "Config: variable updated: " << key;
                try {
                    if(key == "output device") {
                        const auto& sn = boost::get<std::string>(val);
                        if(sinkName == sn)
                            LOG(logject) << "Chosen output same as current. Not reinitialising.";
                        else
                            setPlayerSink(sn);
                    }
                    else
                        ERROR_LOG(logject) << "Config: Unknown key: " << key;
                }
                catch(boost::bad_get&) {
                    ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
                }
            };
            varUpdate.connect(fun);
            c.iterateNodes([&] (const Config::Conf::NodeMap::value_type& node) {
                TRACE_LOG(logject) << "Config: variable loaded: " << node.first;
                fun(node.first, node.second);
            });
        });
    }

    void addOutputDevice(Factory fact, const DeviceName& device) {
        lock_guard l(mu);
        auto it = outputFactories.find(device);
        if(it == outputFactories.end()) {
            outputFactories.emplace(device, fact);
        }
    }

    const std::string& currentSinkName() {
        lock_guard l(mu);
        return sinkName;
    }

    std::unique_ptr<PlayerSink> createPlayerSink() {
        TRACE_LOG(logject) << "Creating player sink";
        lock_guard l(mu);
        if(!fact) {
            TRACE_LOG(logject) << "Sink factory not valid";
            return nullptr;
        }

        return fact();
    }

    void setPlayerSink(const std::string& sinkname) {
        unique_lock l(mu);
        LOG(logject) << "Attempting to get player sink factory: " << sinkname;
        auto it = outputFactories.find(sinkname);

        if(it == outputFactories.end()) {
            BOOST_THROW_EXCEPTION(DeviceNotFoundException() << ErrorTag::DeviceName(sinkname));
        }
        sinkName = sinkname;
        fact = std::bind(it->second, it->first);
        l.unlock();
        playerSinkChanged();
    }

    Signals::Output::PlayerSinkChanged& getPlayerSinkChangedSignal() {
        return playerSinkChanged;
    }

private:
    mutex mu;
    std::string sinkName;
    std::function<std::unique_ptr<Melosic::Output::PlayerSink>()> fact;
    std::map<DeviceName, Factory> outputFactories;
    Config::Conf conf{"Output"};
    PlayerSinkChanged playerSinkChanged;
    Logger::Logger logject{logging::keywords::channel = "Output::Manager"};
    friend class Manager;
};

Manager::Manager(Config::Manager& confman)
    : pimpl(new impl(confman)) {}

Manager::~Manager() {}

void Manager::addOutputDevice(Factory fact, const DeviceName& avail) {
    pimpl->addOutputDevice(fact, avail);
}

const std::string& Manager::currentSinkName() const {
    return pimpl->currentSinkName();
}

std::unique_ptr<PlayerSink> Manager::createPlayerSink() {
    return std::move(pimpl->createPlayerSink());
}

Signals::Output::PlayerSinkChanged& Manager::getPlayerSinkChangedSignal() const {
    return pimpl->getPlayerSinkChangedSignal();
}

} // namespace Output
} // namespace Melosic
