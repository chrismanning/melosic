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
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;
#include <boost/exception/diagnostic_information.hpp>
#include <boost/mpl/find.hpp>
namespace { namespace mpl = boost::mpl; }

#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
namespace json = rapidjson;

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

    VarType VarFromJson(json::Value& val) {
        if(val.IsString())
            return VarType(std::string(val.GetString()));
        else if(val.IsInt64())
            return VarType(val.GetInt64());
        else if(val.IsDouble())
            return VarType(val.GetDouble());
        else if(val.IsBool())
            return VarType(val.GetBool());
        assert(false);
    }

    Conf ConfFromJson(json::Value& val) {
        assert(val.IsObject());
        auto members = boost::make_iterator_range(val.MemberBegin(), val.MemberEnd());
        Conf c;
        for(auto& member : members) {
            if(member.value.IsObject())
                c.children.emplace(member.name.GetString(), ConfFromJson(member.value));
            else
                c.nodes.emplace(member.name.GetString(), VarFromJson(member.value));
        }
        return std::move(c);
    }

    void loadConfig() {
        unique_lock l(mu);
        assert(!confPath.empty());
        if(!fs::exists(confPath)) {
            l.unlock();
            loaded(std::ref(conf));
            saveConfig();
            return;
        }

        fs::ifstream ifs(confPath);
        assert(ifs.good());

        ifs.seekg(0, std::ios::end);
        char confstring[static_cast<std::streamoff>(ifs.tellg())+1];
        ifs.seekg(0, std::ios::beg);
        ifs.read(confstring, sizeof(confstring)-1);
        ifs.close();
        confstring[sizeof(confstring)-1] = 0;
        TRACE_LOG(logject) << confstring;

        json::Document rootjson;
        rootjson.ParseInsitu<0>(confstring);

        if(!rootjson.HasParseError()) {
            Conf tmp_conf = ConfFromJson(rootjson);

            using std::swap;
            swap(conf, tmp_conf);
        }
        else
            ERROR_LOG(logject) << "JSON parse error: " << rootjson.GetParseError();

        l.unlock();
        loaded(std::ref(conf));
        saveConfig();
    }

    template <typename T>
    static constexpr int TypeIndex = mpl::index_of<VarType::types, T>::type::value;
    json::Value JsonFromVar(const VarType& var) {
        switch(var.which()) {
            case TypeIndex<std::string>: {
                const auto& tmp = boost::get<std::string>(var);
                return json::Value(tmp.c_str(),
                                   tmp.size(), poolAlloc);
            }
            case TypeIndex<bool>:
                return json::Value(boost::get<bool>(var));
            case TypeIndex<int64_t>:
                return json::Value(boost::get<int64_t>(var));
            case TypeIndex<double>:
                return json::Value(boost::get<double>(var));
        }
        assert(false);
    }

    json::Value JsonFromNodes(const Conf::NodeMap& nodes) {
        json::Value v(json::kObjectType);
        for(const Conf::NodeMap::value_type& pair: nodes) {
            v.AddMember(pair.first.c_str(), std::move(JsonFromVar(pair.second)), poolAlloc);
        }
        return v;
    }

    json::Value JsonFromConf(const Conf& c) {
        json::Value obj(JsonFromNodes(c.nodes));
        for(const Conf::ChildMap::value_type& pair: c.children) {
            obj.AddMember(pair.first.c_str(), std::move(JsonFromConf(pair.second)), poolAlloc);
        }
        return obj;
    }

    void saveConfig() {
        lock_guard l(mu);

        assert(!confPath.empty());

        json::Document rootjson(JsonFromConf(conf));
        json::StringBuffer strbuf;
        json::PrettyWriter<json::StringBuffer> writer(strbuf);
        rootjson.Accept(writer);

        std::string stringified(strbuf.GetString());
        fs::ofstream ofs(confPath);
        assert(ofs.good());
        ofs << stringified << std::endl;
    }

    Mutex mu;
    json::Value::AllocatorType poolAlloc;
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

Conf& Conf::merge(Conf c) {
    nodes.insert(c.nodes.begin(), c.nodes.end());
    children.insert(c.children.begin(), c.children.end());
    for(auto&& child : c.children) {
        getChild(child.first).merge(child.second);
    }
    return *this;
}

void Conf::addDefaultFunc(Conf::DefaultFunc func) {
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
}

// namespace Melosic
