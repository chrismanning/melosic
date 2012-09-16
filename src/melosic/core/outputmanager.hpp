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

#include <memory>
#include <list>
#include <map>

#include <melosic/managers/output/pluginterface.hpp>

namespace Melosic {
namespace Output {

class OutputDeviceName {
public:
    OutputDeviceName(std::string name) : OutputDeviceName(name, "") {}
    OutputDeviceName(std::string name, std::string desc) : name(name), desc(desc) {}

    const std::string& getName() const {
        return name;
    }

    const std::string& getDesc() const {
        return desc;
    }

    bool operator<(const OutputDeviceName& b) const {
        return name < b.name;
    }

    friend std::ostream& operator<<(std::ostream&, const OutputDeviceName&);

private:
    const std::string name;
    const std::string desc;
};

typedef std::function<std::unique_ptr<IDeviceSink>(const OutputDeviceName&)> Factory;

inline std::ostream& operator<<(std::ostream& out, const OutputDeviceName& b) {
    return out << b.name + ": " + b.desc;
}

class OutputManager {
public:
    typedef std::map<OutputDeviceName, Factory> FactoryMap;

    OutputManager();
    ~OutputManager();
    void addFactory(Factory fact, std::initializer_list<std::string> avail);
    void addFactory(Factory fact, std::list<OutputDeviceName> avail);
    std::unique_ptr<IDeviceSink> getOutputDevice(const std::string& name);
    const FactoryMap& getFactories();
private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
}

#endif // MELOSIC_OUTPUTMANAGER_HPP
