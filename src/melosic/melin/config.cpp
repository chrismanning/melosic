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
#include <boost/mpl/assert.hpp>

#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>

#include "config.hpp"

#ifdef _POSIX_VERSION
#include <wordexp.h>
#endif

namespace Melosic {
static Logger::Logger logject(logging::keywords::channel = "Config");
namespace Config {

static const boost::filesystem::path ConfFile("melosic.conf");

struct Loaded : Signals::Signal<Signals::Config::Loaded> {
    using Super::Signal;
};

struct VariableUpdated : Signals::Signal<Signals::Config::VariableUpdated> {
    using Super::Signal;
};

struct Manager::impl {
    impl() : conf("root") {
    #ifdef _POSIX_VERSION
        wordexp_t exp_result;
        wordexp(dirs[UserDir].c_str(), &exp_result, 0);
        dirs[UserDir] = exp_result.we_wordv[0];
        wordfree(&exp_result);
    #endif
        loaded.connect([this] (Conf&) {
            saveConfig();
        });
    }

    ~impl() {
        try {
            saveConfig();
        }
        catch(...) {
            std::clog << boost::current_exception_diagnostic_information() << std::endl;
        }
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
        if(confPath.empty())
            confPath = dirs[chooseDir()] / ConfFile;
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
    Conf conf;
    Loaded loaded;
};

Manager::Manager() : pimpl(new impl) {}

Manager::~Manager() {}

void Manager::loadConfig() {
    pimpl->loadConfig();
}

void Manager::saveConfig() {
    pimpl->saveConfig();
}

Signals::Config::Loaded& Manager::getLoadedSignal() const{
    return pimpl->loaded;
}

Conf::Conf(std::string name) : name(std::move(name)) {}

Conf::Conf(const Conf& b) :
    children(b.children),
    nodes(b.nodes),
    name(b.name),
    resetDefault(b.resetDefault)
{}

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
    variableUpdated(key, value);
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
    resetDefault = func;
}

void Conf::resetToDefault() {
    if(!resetDefault)
        return;
    auto tmp_sig(std::move(variableUpdated));
    DefaultFunc tmp_fun = resetDefault;
    *this = resetDefault();
    variableUpdated = std::move(tmp_sig);
    resetDefault = std::move(tmp_fun);
}

Signals::Config::VariableUpdated& Conf::getVariableUpdatedSignal() {
    return variableUpdated;
}

void swap(Conf& a, Conf& b)
noexcept(noexcept(swap(a.nodes, b.nodes))
         && noexcept(swap(a.children, b.children))
         && noexcept(swap(a.name, b.name))
         && noexcept(swap(a.resetDefault, b.resetDefault)))
{
    using std::swap;
    swap(a.nodes, b.nodes);
    swap(a.children, b.children);
    swap(a.name, b.name);
    swap(a.resetDefault, b.resetDefault);
}

} // namespace Config
} // namespace Melosic
