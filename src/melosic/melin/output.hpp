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

#include <melosic/common/stream.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/common/range.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/output_signals.hpp>

namespace Melosic {
namespace Output {
struct DeviceName;
class PlayerSink;
class Sink;
}
namespace Config {
class Manager;
}
namespace Plugin {
class Manager;
}

namespace Output {
typedef std::function<std::unique_ptr<PlayerSink>(const DeviceName&)> Factory;

class Manager {
public:
    explicit Manager(Config::Manager&);
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;

    MELOSIC_EXPORT void addOutputDevice(Factory fact, const Output::DeviceName& avail);
    template <typename DeviceList>
    void addOutputDevices(Factory fact, DeviceList avail) {
        for(const DeviceName& device : avail) {
            addOutputDevice(fact, device);
        }
    }

    const std::string& currentSinkName() const;

    std::unique_ptr<PlayerSink> createPlayerSink();
    Signals::Output::PlayerSinkChanged& getPlayerSinkChangedSignal() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class Sink : public IO::Sink {
public:
    virtual ~Sink() {}
    virtual const std::string& getSinkName() = 0;
    virtual void prepareSink(Melosic::AudioSpecs& as) = 0;
};

enum class DeviceState {
    Error,
    Ready,
    Playing,
    Paused,
    Stopped,
    Initial,
};

class PlayerSink : public Sink {
public:
    virtual ~PlayerSink() {}
    virtual const Melosic::AudioSpecs& currentSpecs() = 0;
    virtual const std::string& getSinkDescription() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual DeviceState state() = 0;
};

struct DeviceName {
    DeviceName(std::string name) : DeviceName(name, "") {}
    DeviceName(std::string name, std::string desc) : name(name), desc(desc) {}
    const std::string& getName() const {
        return name;
    }
    const std::string& getDesc() const {
        return desc;
    }
    bool operator<(const DeviceName& b) const {
        return name < b.name;
    }
    friend std::ostream& operator<<(std::ostream&, const DeviceName&);

private:
    const std::string name, desc;
};
inline std::ostream& operator<<(std::ostream& out, const DeviceName& b) {
    return out << b.name + (b.desc.size() ? (": " + b.desc) : "");
}

} // namespace Output
} // namespace Melosic

#endif // MELOSIC_OUTPUTMANAGER_HPP
