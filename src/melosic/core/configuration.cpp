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

#include "configuration.hpp"

namespace Melosic {

Configuration::Configuration(const std::string& name) : name(name) {}

Configuration::Configuration(const Configuration& b) : children(b.children), nodes(b.nodes), name(b.name) {}

Configuration& Configuration::operator=(const Configuration& b) {
    children = b.children;
    nodes = b.nodes;
    name = b.name;

    return *this;
}

const std::string& Configuration::getName() const {
    return name;
}

bool Configuration::existsNode(const std::string& key) {
    try {
        getNode(key);
        return true;
    }
    catch(KeyNotFoundException& e) {
        return false;
    }
}

bool Configuration::existsChild(const std::string &key) {
    try {
        getChild(key);
        return true;
    }
    catch(ChildNotFoundException& e) {
        return false;
    }
}

const Configuration::VarType& Configuration::getNode(const std::string& key) {
    auto it = nodes.find(key);
    if(it != nodes.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(KeyNotFoundException() << ErrorTag::ConfigKey(key));
    }
}

Configuration& Configuration::getChild(const std::string& key) {
    auto it = children.find(key);
    if(it != children.end()) {
        return *(it->second);
    }
    else {
        BOOST_THROW_EXCEPTION(ChildNotFoundException() << ErrorTag::ConfigChild(key));
    }
}

Configuration& Configuration::putChild(const std::string& key, const Configuration& child) {
    ChildMap::iterator it(children.find(key));
    if(it != children.end())
        children.erase(it);
    std::string tmp(key);
    return *children.insert(tmp, child.clone()).first->second;
}

const Configuration::VarType& Configuration::putNode(const std::string& key, const VarType& value) {
    variableUpdated(key, value);
    return nodes[key] = value;
}

boost::signals2::connection Configuration::connectVariableUpdateSlot(const VarUpdateSignal::slot_type &slot) {
    return variableUpdated.connect(slot);
}

} //end namespace Melosic
