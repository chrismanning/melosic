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

#ifndef MELOSIC_OUTPUTMANAGER_HPP
#define MELOSIC_OUTPUTMANAGER_HPP

#include <memory>
#include <functional>
#include <list>

#include <melosic/common/audiospecs.hpp>
#include <melosic/common/range.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/output_signals.hpp>

namespace asio {
class io_service;
}

namespace Melosic {
namespace Output {
struct device_descriptor;
}
namespace Config {
class Manager;
}
namespace AudioIO {
struct AudioOutputBase;
}
namespace Core {
class Kernel;
}

namespace Output {
typedef std::function<std::unique_ptr<AudioIO::AudioOutputBase>(asio::io_service&, device_descriptor)> ASIOFactory;

class Manager final {
    explicit Manager(const std::shared_ptr<Config::Manager>&, asio::io_service&);
    friend class Core::Kernel;

  public:
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;

    MELOSIC_EXPORT void addOutputDevice(ASIOFactory fact, const Output::device_descriptor& avail);
    template <typename DeviceList> void addOutputDevices(ASIOFactory fact, DeviceList avail) {
        for(const device_descriptor& device : avail) {
            addOutputDevice(fact, device);
        }
    }

    const std::string& currentSinkName() const;

    std::unique_ptr<AudioIO::AudioOutputBase> createASIOSink() const;
    Signals::Output::PlayerSinkChanged& getPlayerSinkChangedSignal() const;

  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

enum class DeviceState {
    Error,
    Ready,
    Playing,
    Paused,
    Stopped,
    Initial,
};

struct device_descriptor {
    device_descriptor(std::string name) : name(name) {
    }

    device_descriptor(std::string name, std::string desc) : name(name), desc(desc) {
    }

    const std::string& getName() const {
        return name;
    }

    const std::string& getDesc() const {
        return desc;
    }

    bool operator<(const device_descriptor& b) const {
        return name < b.name;
    }

    template <typename CharT, typename TraitsT>
    friend std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>&,
                                                          const device_descriptor&);

  private:
    const std::string name, desc;
};

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& out, const device_descriptor& b) {
    return out << b.name + (b.desc.size() ? (": " + b.desc) : "");
}

struct provider {
    virtual ~provider() {
    }

    virtual std::vector<std::string> available_devices() const = 0;
};

} // namespace Output
} // namespace Melosic

#endif // MELOSIC_OUTPUTMANAGER_HPP
