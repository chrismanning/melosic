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

#include <map>

#include <melosic/core/outputmanager.hpp>
#include <melosic/common/error.hpp>

namespace Melosic {
namespace Output {

class OutputManager::impl {
public:
    template<class List>
    void addFactory(Factory fact, List avail) {
        for(auto device : avail) {
            auto it = devices.find(device);
            if(it == devices.end()) {
                devices.insert(decltype(devices)::value_type(device, fact));
            }
        }
    }

//    std::unique_ptr<IDeviceSink> getDefaultOutputDevice() {
//        return 0;
//    }

    std::unique_ptr<IDeviceSink> getOutputDevice(const std::string& name) {
        auto it = devices.find(name);

        if(it == devices.end()) {
            throw Exception("No such output device");
        }

        return std::move(std::unique_ptr<IDeviceSink>(it->second(it->first)));
    }

    const std::map<OutputDeviceName, Factory>& getFactories() {
        return devices;
    }

private:
    std::map<OutputDeviceName, Factory> devices;
};

OutputManager::OutputManager() : pimpl(new impl) {}

OutputManager::~OutputManager() {}

void OutputManager::addFactory(Factory fact, std::initializer_list<std::string> avail) {
    pimpl->addFactory(fact, avail);
}

void OutputManager::addFactory(Factory fact, std::list<OutputDeviceName> avail) {
    pimpl->addFactory(fact, avail);
}

std::unique_ptr<IDeviceSink> OutputManager::getOutputDevice(const std::string& name) {
    return pimpl->getOutputDevice(name);
}

const std::map<OutputDeviceName, Factory>& OutputManager::getFactories() {
    return pimpl->getFactories();
}

}
}
