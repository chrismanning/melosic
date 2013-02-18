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

#include <wordexp.h>

#include <map>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/serialize_ptr_map.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <melosic/melin/sigslots/signals_fwd.hpp>
#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include "config.hpp"

template class boost::variant<std::string, bool, int64_t, double, std::vector<uint8_t> >;

namespace Melosic {
namespace Config {

class Configuration : public Config<Configuration> {
public:
    Configuration() : Config("root") {}
};

static const boost::filesystem::path ConfFile("melosic.conf");

class Manager::impl {
    impl(Slots::Manager& slotman) : loaded(slotman.get<Signals::Config::Loaded>()) {
        wordexp_t exp_result;
        wordexp(dirs[UserDir].c_str(), &exp_result, 0);
        dirs[UserDir] = exp_result.we_wordv[0];
        wordfree(&exp_result);
    }

    int chooseDir() {
        if(fs::exists(dirs[CurrentDir] / ConfFile)) {
            return CurrentDir;
        }
        if(fs::exists(fs::absolute(dirs[UserDir]) / ConfFile)) {
            return UserDir;
        }
        if(fs::exists(dirs[SystemDir] / ConfFile)) {
            return SystemDir;
        }
        if(!fs::exists(dirs[UserDir]))
            fs::create_directories(dirs[UserDir]);
        return UserDir;
    }

    void loadConfig() {
        if(confPath.empty()) {
            switch(chooseDir()) {
                case CurrentDir:
                    confPath = dirs[CurrentDir] / ConfFile;
                    break;
                case UserDir:
                    confPath = dirs[UserDir] / ConfFile;
                    break;
                case SystemDir:
                    confPath = dirs[SystemDir] / ConfFile;
                    break;
            }
        }
        assert(!confPath.empty());
        if(!fs::exists(confPath)) {
            loaded(std::ref(conf));
            return;
        }

        fs::ifstream ifs(confPath);
        assert(ifs.good());
        boost::archive::binary_iarchive ia(ifs);
        ia >> conf;
        loaded(std::ref(conf));
    }

    void saveConfig() {
        assert(!confPath.empty());

        fs::ofstream ofs(confPath);
        assert(ofs.good());
        boost::archive::binary_oarchive oa(ofs);
        oa << conf;
    }

    fs::path confPath;
    Configuration conf;
    Signals::Config::Loaded& loaded;
    friend class Manager;
};

Manager::Manager(Slots::Manager& slotman) : pimpl(new impl(slotman)) {}

Manager::~Manager() {}

void Manager::loadConfig() {
    pimpl->loadConfig();
}

void Manager::saveConfig() {
    pimpl->saveConfig();
}

Base& Manager::addConfigTree(const Base& c) {
    return pimpl->conf.putChild(c.getName(), c);
}

Base& Manager::getConfigRoot() {
    return pimpl->conf;
}

class Base::impl {
public:
    typedef boost::ptr_map<std::string, Base> ChildMap;
    typedef std::map<std::string, VarType> NodeMap;

    impl() {}
    impl(const std::string& name) : name(name) {}

    impl* clone() {
        return new impl(*this);
    }

    const VarType& putNode(const std::string& key, const VarType& value) {
        variableUpdated(key, value);
        return nodes[key] = value;
    }

private:
    ChildMap children;
    NodeMap nodes;
    std::string name;
    friend class Base;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & children;
        ar & nodes;
        ar & name;
    }
    Signals::Config::VariableUpdated variableUpdated;
    std::function<Base&()> resetDefault;
};

Base::Base(const std::string& name) : pimpl(new impl(name)) {}

Base::Base() : pimpl(new impl) {}

Base::Base(const Base& b) : pimpl(b.pimpl->clone()) {}

Base& Base::operator=(const Base& b) {
    pimpl.reset(b.pimpl->clone());

    return *this;
}

const std::string& Base::getName() const {
    return pimpl->name;
}

bool Base::existsNode(const std::string& key) const {
    try {
        getNode(key);
        return true;
    }
    catch(KeyNotFoundException& e) {
        return false;
    }
}

bool Base::existsChild(const std::string& key) const {
    try {
        getChild(key);
        return true;
    }
    catch(ChildNotFoundException& e) {
        return false;
    }
}

const VarType& Base::getNode(const std::string& key) const {
    auto it = pimpl->nodes.find(key);
    if(it != pimpl->nodes.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(KeyNotFoundException() << ErrorTag::ConfigKey(key));
    }
}

Base& Base::getChild(const std::string& key) {
    auto it = pimpl->children.find(key);
    if(it != pimpl->children.end()) {
        return *(it->second);
    }
    else {
        BOOST_THROW_EXCEPTION(ChildNotFoundException() << ErrorTag::ConfigChild(key));
    }
}

const Base& Base::getChild(const std::string& key) const {
    auto it = pimpl->children.find(key);
    if(it != pimpl->children.end()) {
        return *(it->second);
    }
    else {
        BOOST_THROW_EXCEPTION(ChildNotFoundException() << ErrorTag::ConfigChild(key));
    }
}

Base& Base::putChild(const std::string& key, const Base& child) {
    impl::ChildMap::iterator it(pimpl->children.find(key));
    if(it != pimpl->children.end())
        pimpl->children.erase(it);
    std::string tmp(key);
    return *pimpl->children.insert(tmp, new_clone(child)).first->second;
}

const VarType& Base::putNode(const std::string& key, const VarType& value) {
    return pimpl->putNode(key, value);
}

ForwardRange<Base* const> Base::getChildren() {
    return pimpl->children | map_values;
}

ForwardRange<const Base* const> Base::getChildren() const {
    return pimpl->children | map_values;
}

ForwardRange<Base::impl::NodeMap::value_type> Base::getNodes() {
    return pimpl->nodes;
}

ForwardRange<const Base::impl::NodeMap::value_type> Base::getNodes() const {
    return pimpl->nodes;
}

void Base::addDefaultFunc(std::function<Base&()> func) {
    pimpl->resetDefault = func;
}

void Base::resetToDefault() {
    if(pimpl->resetDefault) {
        auto tmp_sig = std::move(pimpl->variableUpdated);
        auto tmp_fun = pimpl->resetDefault;
        *this = pimpl->resetDefault();
        pimpl->variableUpdated = std::move(tmp_sig);
        pimpl->resetDefault = std::move(tmp_fun);
    }
}

template<class Archive>
void Base::serialize(Archive& ar, const unsigned int /*version*/) {
    ar & *pimpl;
}

template void Base::serialize<boost::archive::binary_iarchive>(
    boost::archive::binary_iarchive& ar,
    const unsigned int file_version
);
template void Base::serialize<boost::archive::binary_oarchive>(
    boost::archive::binary_oarchive& ar,
    const unsigned int file_version
);

Base* new_clone(const Base& conf) {
    return conf.clone();
}

template <>
Signals::Config::VariableUpdated& Base::get<Signals::Config::VariableUpdated>() {
    return pimpl->variableUpdated;
}

template Signals::Config::VariableUpdated& Base::get<Signals::Config::VariableUpdated>();

} // namespace Config
} // namespace Melosic
