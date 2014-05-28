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
#include <asio/io_service.hpp>

#include <melosic/common/error.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/common/signal_core.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/asio/audio_io.hpp>

#include "output.hpp"

namespace Melosic {
namespace Output {

struct PlayerSinkChanged : Signals::Signal<Signals::Output::PlayerSinkChanged> {
    using Super::Signal;
};

class Manager::impl {
public:
    impl(Config::Manager& confman, ASIO::io_service& io_service) : io_service(io_service) {
        conf.putNode("output device", "default"s);
        conf.putNode("buffer time", 1000);
        confman.getLoadedSignal().connect(&impl::loadedSlot, this);
    }

    void loadedSlot(boost::synchronized_value<Config::Conf>& base) {
        TRACE_LOG(logject) << "Output conf loaded";

        auto c = base->createChild("Output"s, conf);
        c->merge(conf);
        c->setDefault(conf);
        c->iterateNodes([&] (const std::string& key, auto&& var) {
            TRACE_LOG(logject) << "Config: variable loaded: " << key;
            variableUpdateSlot(key, var);
        });
        m_signal_connections.emplace_back(c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this));
    }

    void variableUpdateSlot(const Config::Conf::node_key_type& key, const Config::VarType& val) {
        using std::get;
        TRACE_LOG(logject) << "Config: variable updated: " << key;
        try {
            if(key == "output device") {
                const auto& sn = get<std::string>(val);
                if(sinkName == sn)
                    LOG(logject) << "Chosen output same as current. Not reinitialising.";
                else
                    setASIOSink(sn);
            }
            else
                ERROR_LOG(logject) << "Config: Unknown key: " << key;
        }
        catch(boost::bad_get&) {
            ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        }
    }

    void addOutputDevice(ASIOFactory fact, const DeviceName& device) {
        lock_guard l(mu);
        auto it = asioOutputFactories.find(device);
        if(it == asioOutputFactories.end()) {
            asioOutputFactories.emplace(device, fact);
        }
    }

    const std::string& currentSinkName() {
        lock_guard l(mu);
        return sinkName;
    }

    std::unique_ptr<ASIO::AudioOutputBase> createASIOSink() {
        TRACE_LOG(logject) << "Creating player sink " << sinkName;
        lock_guard l(mu);

        auto it = asioOutputFactories.find(sinkName);

        if(it == asioOutputFactories.end()) {
            BOOST_THROW_EXCEPTION(DeviceNotFoundException() << ErrorTag::DeviceName(sinkName));
            return nullptr;
        }

        return it->second(io_service, sinkName);
    }

    void setASIOSink(std::string sinkname) {
        unique_lock l(mu);
        sinkName = std::move(sinkname);
        l.unlock();
        playerSinkChanged();
    }

    Signals::Output::PlayerSinkChanged& getPlayerSinkChangedSignal() {
        return playerSinkChanged;
    }

private:
    mutex mu;
    ASIO::io_service& io_service;
    std::string sinkName;
    std::map<DeviceName, ASIOFactory> asioOutputFactories;
    Config::Conf conf{"Output"};
    PlayerSinkChanged playerSinkChanged;
    Logger::Logger logject{logging::keywords::channel = "Output::Manager"};
    std::vector<Signals::ScopedConnection> m_signal_connections;
    friend class Manager;
};

Manager::Manager(Config::Manager& confman, asio::io_service& io_service)
    : pimpl(new impl(confman, io_service)) {}

Manager::~Manager() {}

void Manager::addOutputDevice(ASIOFactory fact, const DeviceName& avail) {
    pimpl->addOutputDevice(fact, avail);
}

const std::string& Manager::currentSinkName() const {
    return pimpl->currentSinkName();
}

std::unique_ptr<ASIO::AudioOutputBase> Manager::createASIOSink() const {
    return std::move(pimpl->createASIOSink());
}

Signals::Output::PlayerSinkChanged& Manager::getPlayerSinkChangedSignal() const {
    return pimpl->getPlayerSinkChangedSignal();
}

} // namespace Output
} // namespace Melosic
