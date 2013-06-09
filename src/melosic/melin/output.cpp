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

#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/common/string.hpp>
#include <melosic/common/error.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/melin/plugin.hpp>

#include "output.hpp"

template class std::function<std::unique_ptr<Melosic::Output::PlayerSink>(const Melosic::Output::DeviceName&)>;

namespace Melosic {
namespace Output {

class Manager::impl {
public:
    impl(Slots::Manager& slotman) :
        playerSinkChanged(slotman.get<Signals::Output::PlayerSinkChanged>()),
        logject(logging::keywords::channel = "Output::Manager")
    {
        slotman.get<Signals::Config::Loaded>().connect([this](Config::Base& base) {
            TRACE_LOG(logject) << "Output conf loaded";
            auto& c = base.existsChild("Output")
                    ? base.getChild("Output")
                    : base.putChild("Output", conf);
            static_cast<Conf&>(c).outputFactories = &outputFactories;
            c.addDefaultFunc([=]() -> Config::Base& { return *conf.clone(); });
            auto& varUpdate = c.get<Signals::Config::VariableUpdated>();
            varUpdate.connect([this](const std::string& key, const Config::VarType& val) {
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
            });
            for(const auto& node : c.getNodes()) {
                TRACE_LOG(logject) << "Config: variable loaded: " << node.first;
                varUpdate(node.first, node.second);
            }
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
        lock_guard l(mu);
        if(!fact) {
            return nullptr;
        }

        return fact();
    }

    void setPlayerSink(const std::string& sinkname) {
        unique_lock l(mu);
        LOG(logject) << "Attempting to open player sink: " << sinkname;
        auto it = outputFactories.find(sinkname);

        if(it == outputFactories.end()) {
            BOOST_THROW_EXCEPTION(DeviceNotFoundException() << ErrorTag::DeviceName(sinkname));
        }
        sinkName = sinkname;
        fact = std::bind(it->second, it->first);
        l.unlock();
        playerSinkChanged();
    }

private:
    mutex mu;
    std::string sinkName;
    std::function<std::unique_ptr<Melosic::Output::PlayerSink>()> fact;
    std::map<DeviceName, Factory> outputFactories;
    Conf conf;
    Signals::Output::PlayerSinkChanged& playerSinkChanged;
    Logger::Logger logject;
    friend class Manager;
    friend class Conf;
};

Manager::Manager(Slots::Manager& slotman)
    : pimpl(new impl(slotman)) {}

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

Conf::Conf() : Config::Config("Output") {
    putNode("output device", "iec958:CARD=PCH,DEV=0"_str);
}

} // namespace Output
} // namespace Melosic

BOOST_CLASS_EXPORT_IMPLEMENT(Melosic::Output::Conf)
