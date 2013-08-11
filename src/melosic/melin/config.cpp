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
                c.putChild(member.name.GetString(), ConfFromJson(member.value));
            else
                c.putNode(member.name.GetString(), VarFromJson(member.value));
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
            Conf tmp_conf = std::move(ConfFromJson(rootjson));

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

    json::Value JsonFromNodes(Conf::ConstNodeRange nodes) {
        json::Value v(json::kObjectType);
        for(const Conf::NodeMap::value_type& pair : nodes) {
            v.AddMember(pair.first.c_str(), std::move(JsonFromVar(pair.second)), poolAlloc);
        }
        return v;
    }

    json::Value JsonFromConf(const Conf& c) {
        json::Value obj(JsonFromNodes(c.getNodes()));
        for(const auto& conf : c.getChildren()) {
            obj.AddMember(conf.getName().c_str(), std::move(JsonFromConf(conf)), poolAlloc);
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
    Signals::Config::VariableUpdated& getVariableUpdatedSignal();

    impl(std::string name) : name(std::move(name)) {}

    impl(const impl& b)
        : children(b.children),
          nodes(b.nodes),
          name(b.name),
          resetDefault(b.resetDefault)
    {}

    impl(impl&&) = default;

    mutable Mutex mu;
    Conf::ChildMap children;
    Conf::NodeMap nodes;
    std::string name;
    Conf::DefaultFunc resetDefault;

    const std::string& getName();
    bool existsNode(std::string key);
    bool existsChild(std::string key);
    const VarType& getNode(std::string key);
    Conf& getChild(std::string key);
    Conf& putChild(std::string key, Conf child);
    VarType& putNode(std::string key, VarType value);
    Conf::ChildRange getChildren();
    Conf::ConstChildRange getChildren() const;
    Conf::NodeRange getNodes();

    void merge(impl& c);

    void addDefaultFunc(Conf::DefaultFunc func);
    Conf resetToDefault();
};

Conf::Conf() : Conf(""s) {}

Conf::Conf(Conf&& b) {
    using std::swap;
    swap(*this, b);
}

Conf::Conf(std::string name) :
    pimpl(std::make_unique<impl>(std::move(name)))
{}

Conf::~Conf() {}

Conf::Conf(const Conf& b) {
    lock_guard l(b.pimpl->mu);
    pimpl = std::make_unique<impl>(*b.pimpl);
}

Conf& Conf::operator=(Conf b) {
    lock_guard l(b.pimpl->mu);
    using std::swap;
    swap(*this, b);
    return *this;
}

const std::string& Conf::impl::getName() {
    lock_guard l(mu);
    return name;
}

bool Conf::impl::existsNode(std::string key) {
    try {
        getNode(key);
        return true;
    }
    catch(KeyNotFoundException& e) {
        return false;
    }
}

bool Conf::impl::existsChild(std::string key) {
    try {
        getChild(key);
        return true;
    }
    catch(ChildNotFoundException& e) {
        return false;
    }
}

const VarType& Conf::impl::getNode(std::string key) {
    lock_guard l(mu);
    auto it = nodes.find(key);
    if(it != nodes.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(KeyNotFoundException() << ErrorTag::ConfigKey(key));
    }
}

Conf& Conf::impl::getChild(std::string key) {
    lock_guard l(mu);
    auto it = children.find(key);
    if(it != children.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(ChildNotFoundException() << ErrorTag::ConfigChild(key));
    }
}

Conf& Conf::impl::putChild(std::string key, Conf child) {
    lock_guard l(mu);
    return children[std::move(key)] = std::move(child);
}

VarType& Conf::impl::putNode(std::string key, VarType value) {
    variableUpdated(key, value);
    lock_guard l(mu);
    return nodes[std::move(key)] = std::move(value);
}

Conf::ChildRange Conf::impl::getChildren() {
    lock_guard l(mu);
    return children | map_values;
}

Conf::ConstChildRange Conf::impl::getChildren() const {
    lock_guard l(mu);
    return children | map_values;
}

Conf::NodeRange Conf::impl::getNodes() {
    lock_guard l(mu);
    return nodes;
}

void Conf::impl::merge(Conf::impl& c) {
    std::lock(mu, c.mu);
    nodes.insert(c.nodes.begin(), c.nodes.end());
    children.insert(c.children.begin(), c.children.end());
    mu.unlock();
    for(auto& child : c.children) {
        getChild(child.first).merge(child.second);
    }
    c.mu.unlock();
}

void Conf::impl::addDefaultFunc(Conf::DefaultFunc func) {
    lock_guard l(mu);
    resetDefault = func;
}

Conf Conf::impl::resetToDefault() {
    lock_guard l(mu);
    if(!resetDefault)
        return Conf();
    return std::move(resetDefault());
}

Signals::Config::VariableUpdated& Conf::impl::getVariableUpdatedSignal() {
    return variableUpdated;
}

const std::string& Conf::getName() const {
    return pimpl->getName();
}
bool Conf::existsNode(std::string key) const {
    return pimpl->existsNode(std::move(key));
}
bool Conf::existsChild(std::string key) const {
    return pimpl->existsChild(std::move(key));
}
const VarType& Conf::getNode(std::string key) const{
    return pimpl->getNode(std::move(key));
}
Conf& Conf::getChild(std::string key) {
    return pimpl->getChild(std::move(key));
}
const Conf& Conf::getChild(std::string key) const{
    return pimpl->getChild(std::move(key));
}
Conf& Conf::putChild(std::string key, Conf child) {
    return pimpl->putChild(std::move(key), std::move(child));
}
VarType& Conf::putNode(std::string key, VarType value) {
    return pimpl->putNode(std::move(key), std::move(value));
}
Conf::ChildRange Conf::getChildren() {
    return pimpl->getChildren();
}
Conf::ConstChildRange Conf::getChildren() const {
    return static_cast<impl const*>(pimpl.get())->getChildren();
}
Conf::NodeRange Conf::getNodes() {
    return pimpl->getNodes();
}
Conf::ConstNodeRange Conf::getNodes() const {
    return pimpl->getNodes();
}

Conf& Conf::merge(const Conf& c) {
    pimpl->merge(*c.pimpl);
    return *this;
}

void Conf::addDefaultFunc(DefaultFunc df) {
    pimpl->addDefaultFunc(std::move(df));
}
void Conf::resetToDefault() {
    *this = pimpl->resetToDefault();
}

Signals::Config::VariableUpdated& Conf::getVariableUpdatedSignal() {
    return pimpl->getVariableUpdatedSignal();
}

void swap(Conf& a, Conf& b)
noexcept(noexcept(swap(a.pimpl, b.pimpl)))
{
    using std::swap;
    swap(a.pimpl, b.pimpl);
}

} // namespace Config
}

// namespace Melosic
