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

#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/common/error.hpp>

#include "output.hpp"

template class std::function<std::unique_ptr<Melosic::Output::PlayerSink>(const Melosic::Output::DeviceName&)>;

namespace Melosic {
namespace Output {

class Manager::impl {
public:
    void addOutputDevice(Factory fact, const DeviceName& device) {
        auto it = outputFactories.find(device);
        if(it == outputFactories.end()) {
            outputFactories.emplace(device, fact);
        }
    }
private:
    std::map<DeviceName, Factory> outputFactories;
    friend class Manager;
};

Manager::Manager() : pimpl(new impl) {}

Manager::~Manager() {}

void Manager::addOutputDevice(Factory fact, const DeviceName& avail) {
    pimpl->addOutputDevice(fact, avail);
}

std::unique_ptr<PlayerSink> Manager::getOutputDevice(const std::string& devicename) {
    auto it = pimpl->outputFactories.find(devicename);

    if(it == pimpl->outputFactories.end()) {
        BOOST_THROW_EXCEPTION(DeviceNotFoundException() << ErrorTag::DeviceName(devicename));
    }

    return it->second(it->first);
}

ForwardRange<const DeviceName> Manager::getOutputDeviceNames() {
    return pimpl->outputFactories | map_keys;
}

} // namespace Output
} // namespace Melosic
