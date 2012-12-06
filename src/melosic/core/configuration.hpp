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

#ifndef MELOSIC_CONFIGURATION_HPP
#define MELOSIC_CONFIGURATION_HPP

#include <memory>
#include <map>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/signals2.hpp>

#include <melosic/common/error.hpp>

namespace Melosic {

class Configuration {
public:
    typedef boost::signals2::signal<void(const std::string&, const std::string&)> VarUpdateSignal;

    Configuration();

    Configuration(const Configuration& b);
    Configuration& operator=(const Configuration& b);

    bool existsNode(const std::string& key);
    bool existsChild(const std::string& key);
    const std::string& getNode(const std::string& key);
    Configuration& getChild(const std::string& key);
    Configuration& putChild(const std::string& key, const Configuration& child);
    const std::string& putNode(const std::string& key, const std::string& value);
    boost::signals2::connection connectVariableUpdateSlot(const VarUpdateSignal::slot_type& slot);

private:
    std::map<std::string, Configuration> children;
    std::map<std::string, std::string> nodes;
    VarUpdateSignal variableUpdated;
};

} //end namespace Melosic

#endif // MELOSIC_CONFIGURATION_HPP
