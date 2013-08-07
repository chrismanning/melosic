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

#include <functional>
#include <map>
#include <thread>
#include <mutex>
typedef std::mutex Mutex;
using lock_guard = std::lock_guard<Mutex>;
using unique_lock = std::unique_lock<Mutex>;

#include <melosic/common/configvar.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;
#include <boost/exception/diagnostic_information.hpp>

#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>

#include "config.hpp"

#include <melosic/common/directories.hpp>

namespace Melosic {
static Logger::Logger logject(logging::keywords::channel = "Config");
namespace Config {

static const fs::path ConfFile("melosic.conf");

struct Loaded : Signals::Signal<Signals::Config::Loaded> {
    using Super::Signal;
};

struct VariableUpdated : Signals::Signal<Signals::Config::VariableUpdated> {
    using Super::Signal;
};

struct Manager::impl {
    impl(fs::path p) : conf("root") {
        if(p.empty())
            BOOST_THROW_EXCEPTION(Exception());

        confPath = std::move(Directories::configHome() / "melosic" / p.filename());

        if(!fs::exists(confPath.parent_path()))
            fs::create_directories(confPath.parent_path());
        TRACE_LOG(logject) << "Conf file path: " << confPath;
    }

    ~impl() {}

    void loadConfig() {
        assert(!confPath.empty());
        if(!fs::exists(confPath)) {
            loaded(std::ref(conf));
            saveConfig();
            return;
        }

        fs::ifstream ifs(confPath);
        assert(ifs.good());
        boost::archive::binary_iarchive ia(ifs);
        ia >> conf;
        loaded(std::ref(conf));
        saveConfig();
    }

    void saveConfig() {
        assert(!confPath.empty());

        fs::ofstream ofs(confPath);
        assert(ofs.good());
        boost::archive::binary_oarchive oa(ofs);
        oa << conf;
    }

    fs::path confPath;
    Conf conf;
    Loaded loaded;
};

Manager::Manager(fs::path p) : pimpl(new impl(std::move(p))) {}

Manager::~Manager() {}

void Manager::loadConfig() {
    pimpl->loadConfig();
}

void Manager::saveConfig() {
    pimpl->saveConfig();
}

Signals::Config::Loaded& Manager::getLoadedSignal() const {
    return pimpl->loaded;
}

struct Conf::impl {
    struct VariableUpdated : Signals::Signal<Signals::Config::VariableUpdated> {
        using Super::Signal;
    };
    VariableUpdated variableUpdated;
    Conf::DefaultFunc resetDefault;
};

Conf::Conf() : Conf(""s) {}

Conf::Conf(Conf&& b) {
    using std::swap;
    swap(*this, b);
}

Conf::Conf(std::string name) :
    pimpl(std::make_unique<impl>()),
    name(std::move(name)) {}

Conf::~Conf() {}

Conf::Conf(const Conf& b) :
    pimpl(std::make_unique<impl>()),
    children(b.children),
    nodes(b.nodes),
    name(b.name)
{
    pimpl->resetDefault = b.pimpl->resetDefault;
}

Conf& Conf::operator=(Conf b) {
    using std::swap;
    swap(*this, b);
    return *this;
}

const std::string& Conf::getName() const {
    return name;
}

bool Conf::existsNode(std::string key) const {
    try {
        getNode(key);
        return true;
    }
    catch(KeyNotFoundException& e) {
        return false;
    }
}

bool Conf::existsChild(std::string key) const {
    try {
        getChild(key);
        return true;
    }
    catch(ChildNotFoundException& e) {
        return false;
    }
}

const VarType& Conf::getNode(std::string key) const {
    auto it = nodes.find(key);
    if(it != nodes.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(KeyNotFoundException() << ErrorTag::ConfigKey(key));
    }
}

Conf& Conf::getChild(std::string key) {
    auto it = children.find(key);
    if(it != children.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(ChildNotFoundException() << ErrorTag::ConfigChild(key));
    }
}

const Conf& Conf::getChild(std::string key) const {
    auto it = children.find(key);
    if(it != children.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(ChildNotFoundException() << ErrorTag::ConfigChild(key));
    }
}

Conf& Conf::putChild(std::string key, Conf child) {
    return children[std::move(key)] = std::move(child);
}

VarType& Conf::putNode(std::string key, VarType value) {
    pimpl->variableUpdated(key, value);
    return nodes[std::move(key)] = std::move(value);
}

Conf::ChildRange Conf::getChildren() {
    return children | map_values;
}

Conf::ConstChildRange Conf::getChildren() const {
    return children | map_values;
}

Conf::NodeRange Conf::getNodes() {
    return nodes;
}

Conf::ConstNodeRange Conf::getNodes() const {
    return nodes;
}

void Conf::addDefaultFunc(std::function<Conf&()> func) {
    pimpl->resetDefault = func;
}

void Conf::resetToDefault() {
    if(!pimpl->resetDefault)
        return;
    *this = std::move(pimpl->resetDefault());
}

Signals::Config::VariableUpdated& Conf::getVariableUpdatedSignal() {
    return pimpl->variableUpdated;
}

void swap(Conf& a, Conf& b)
noexcept(noexcept(swap(a.pimpl, b.pimpl))
         && noexcept(swap(a.nodes, b.nodes))
         && noexcept(swap(a.children, b.children))
         && noexcept(swap(a.name, b.name)))
{
    using std::swap;
    swap(a.pimpl, b.pimpl);
    swap(a.nodes, b.nodes);
    swap(a.children, b.children);
    swap(a.name, b.name);
}

} // namespace Config
} // namespace Melosic
