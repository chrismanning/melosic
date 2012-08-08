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

#ifndef MELOSIC_OUTPUTMANAGER_HPP
#define MELOSIC_OUTPUTMANAGER_HPP

#include <list>

#include <melosic/managers/output/ioutputmanager.hpp>
#include <melosic/managers/output/pluginterface.hpp>

namespace Melosic {
namespace Output {

class OutputManager : public IOutputManager {
public:
    void addOutput(IDeviceSink* dev) {
        devs.push_back(dev);
    }

    IDeviceSink* getDefaultOutput() {
//        auto r = filter!(a => a.getDeviceName().canFind("default"))(devs);
//        enforceEx!Exception(r.count() > 0, "Cannot find default device");
//        return r.front().iod;
        return 0;
    }

private:
    std::list<IDeviceSink*> devs;
};

}
}

#endif // MELOSIC_OUTPUTMANAGER_HPP
