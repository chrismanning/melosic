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

#include <melosic/common/configvar.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;
#include <boost/exception/diagnostic_information.hpp>
#include <boost/mpl/find.hpp>
namespace { namespace mpl = boost::mpl; }
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/format.hpp>
namespace { using boost::format; }

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
    typedef std::mutex Mutex;
    using lock_guard = std::lock_guard<Mutex>;
    using unique_lock = std::unique_lock<Mutex>;

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

    Conf ConfFromJson(json::Value& val, std::string name) {
        assert(val.IsObject());
        auto members = boost::make_iterator_range(val.MemberBegin(), val.MemberEnd());
        Conf c(name);
        for(auto& member : members) {
            std::string name_str(member.name.GetString());
            if(member.value.IsObject()) {
                TRACE_LOG(logject) << "adding child conf from json: " << name_str;
                c.putChild(name_str, ConfFromJson(member.value, std::move(name_str)));
            }
            else {
                TRACE_LOG(logject) << "adding node from json: " << name_str;
                c.putNode(name_str, VarFromJson(member.value));
            }
        }
        return std::move(c);
    }

    void loadConfig() {
        TRACE_LOG(logject) << "loading config from json";

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

        json::Document rootjson;
        rootjson.ParseInsitu<0>(confstring);

        if(!rootjson.HasParseError()) {
            Conf tmp_conf = std::move(ConfFromJson(rootjson, "root"s));

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

    json::Value JsonFromConf(const Conf& c) {
        json::Value obj(json::kObjectType);
        c.iterateNodes([&] (const Conf::NodeMap::value_type& pair) {
            TRACE_LOG(logject) << "adding node to json: " << pair.first;
            obj.AddMember(pair.first.c_str(), std::move(JsonFromVar(pair.second)), poolAlloc);
        });
        c.iterateChildren([&] (const Conf& childconf) {
            TRACE_LOG(logject) << "adding conf to json: " << childconf.getName();
            obj.AddMember(childconf.getName().c_str(), std::move(JsonFromConf(childconf)), poolAlloc);
        });
        return obj;
    }

    void saveConfig() {
        TRACE_LOG(logject) << "saving config as json";

        lock_guard l(mu);
        assert(!confPath.empty());

        json::Document rootjson(JsonFromConf(conf));
        json::StringBuffer strbuf;
        json::PrettyWriter<json::StringBuffer> writer(strbuf);
        rootjson.Accept(writer);

        std::string stringified(strbuf.GetString());
        TRACE_LOG(logject) << "JSON: " << stringified;
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
    typedef boost::shared_mutex Mutex;
    using lock_guard = std::lock_guard<Mutex>;
    using unique_lock = std::unique_lock<Mutex>;
    using shared_lock_guard = boost::shared_lock_guard<Mutex>;

    struct VariableUpdated : Signals::Signal<Signals::Config::VariableUpdated> {
        using Super::Signal;
    };
    VariableUpdated variableUpdated;

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

    bool existsNode(std::string key);
    bool existsChild(std::string key);
    VarType& getNode(std::string key);
    Conf& getChild(std::string key);
    Conf& putChild(std::string key, Conf child);
    VarType& putNode(std::string key, VarType value);
    auto getChildren();
    auto& getNodes();

    void merge(impl& c);

    void addDefaultFunc(Conf::DefaultFunc func);
    Conf resetToDefault();
};

bool Conf::impl::existsNode(std::string key) {
    try {
        shared_lock_guard l(mu);
        return nodes.end() != nodes.find(key);
    }
    catch(...) {
        return false;
    }
}

bool Conf::impl::existsChild(std::string key) {
    try {
        shared_lock_guard l(mu);
        return children.end() != children.find(key);
    }
    catch(...) {
        return false;
    }
}

VarType& Conf::impl::getNode(std::string key) {
    TRACE_LOG(logject) << (format("[%s] getNode(): %s") % name % key);
    shared_lock_guard l(mu);
    auto it = nodes.find(key);
    if(it != nodes.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(KeyNotFoundException() << ErrorTag::ConfigKey(key));
    }
}

Conf& Conf::impl::getChild(std::string key) {
    TRACE_LOG(logject) << (format("[%s] getChild(): %s") % name % key);
    shared_lock_guard l(mu);
    auto it = children.find(key);
    if(it != children.end()) {
        return it->second;
    }
    else {
        BOOST_THROW_EXCEPTION(ChildNotFoundException() << ErrorTag::ConfigChild(key));
    }
}

Conf& Conf::impl::putChild(std::string key, Conf child) {
    TRACE_LOG(logject) << (format("[%s] putChild(): (%s, %s)") % name % key % child.getName());
    lock_guard l(mu);
    return children[std::move(key)] = std::move(child);
}

VarType& Conf::impl::putNode(std::string key, VarType value) {
    TRACE_LOG(logject) << (format("[%s] putNode(): %s") % name % key);
    variableUpdated(key, value);
    lock_guard l(mu);
    return nodes[std::move(key)] = std::move(value);
}

auto Conf::impl::getChildren() {
    return children | map_values;
}

auto& Conf::impl::getNodes() {
    return nodes;
}

void Conf::impl::merge(Conf::impl& c) {
    TRACE_LOG(logject) << (format("[%s] merge(): %s") % name % c.name);
    std::lock(mu, c.mu);
    nodes.insert(c.nodes.begin(), c.nodes.end());
    children.insert(c.children.begin(), c.children.end());
    mu.unlock();
    for(const auto& child : c.children) {
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

Conf::Conf() : Conf(""s) {}

Conf::Conf(std::string name) :
    pimpl(std::make_unique<impl>(std::move(name)))
{}

Conf::Conf(Conf&& b) {
    using std::swap;
    swap(*this, b);
}

Conf::~Conf() {}

Conf::Conf(const Conf& b) {
    impl::lock_guard l(b.pimpl->mu);
    pimpl = std::make_unique<impl>(*b.pimpl);
}

Conf& Conf::operator=(Conf b) {
    impl::lock_guard l(b.pimpl->mu);
    using std::swap;
    swap(*this, b);
    return *this;
}

const std::string& Conf::getName() const noexcept {
    return pimpl->name;
}

bool Conf::existsNode(std::string key) const {
    return pimpl->existsNode(std::move(key));
}

bool Conf::existsChild(std::string key) const {
    return pimpl->existsChild(std::move(key));
}

bool Conf::applyNode(std::string key, std::function<void(NodeMap::mapped_type&)> fun) {
    try {
        auto& node = pimpl->getNode(key);
        impl::lock_guard l(pimpl->mu);
        fun(node);
        return true;
    }
    catch(...) {
        return false;
    }
}

bool Conf::applyNode(std::string key, std::function<void(const NodeMap::mapped_type&)> fun) const {
    try {
        const auto& node = pimpl->getNode(key);
        impl::shared_lock_guard l(pimpl->mu);
        fun(node);
        return true;
    }
    catch(...) {
        return false;
    }
}

bool Conf::applyChild(std::string key, std::function<void(Conf&)> fun) {
    try {
        auto& child = pimpl->getChild(key);
        impl::lock_guard l(pimpl->mu);
        fun(child);
        return true;
    }
    catch(...) {
        return false;
    }
}

bool Conf::applyChild(std::string key, std::function<void(const Conf&)> fun) const {
    try {
        const auto& child = pimpl->getChild(key);
        impl::shared_lock_guard l(pimpl->mu);
        fun(child);
        return true;
    }
    catch(...) {
        return false;
    }
}

void Conf::putChild(std::string key, Conf child) {
    pimpl->putChild(std::move(key), std::move(child));
}

void Conf::putNode(std::string key, VarType value) {
    pimpl->putNode(std::move(key), std::move(value));
}

Conf::ChildMap::size_type Conf::childCount() const noexcept {
    return pimpl->children.size();
}
Conf::NodeMap::size_type Conf::nodeCount() const noexcept {
    return pimpl->nodes.size();
}

void Conf::iterateChildren(std::function<void(const Conf&)> fun) const {
    impl::shared_lock_guard l(pimpl->mu);
    for(const auto& child : pimpl->getChildren())
        fun(child);
}

void Conf::iterateChildren(std::function<void(Conf&)> fun) {
    impl::lock_guard l(pimpl->mu);
    for(auto& child : pimpl->getChildren())
        fun(child);
}

void Conf::iterateNodes(std::function<void(const NodeMap::value_type&)> fun) const {
    impl::shared_lock_guard l(pimpl->mu);
    for(const auto& node : pimpl->getNodes())
        fun(node);
}

void Conf::iterateNodes(std::function<void(NodeMap::value_type&)> fun) {
    impl::lock_guard l(pimpl->mu);
    for(auto& node : pimpl->getNodes())
        fun(node);
}

void Conf::merge(const Conf& c) {
    pimpl->merge(*c.pimpl);
}

void Conf::addDefaultFunc(DefaultFunc df) {
    pimpl->addDefaultFunc(std::move(df));
}
void Conf::resetToDefault() {
    *this = pimpl->resetToDefault();
}

Signals::Config::VariableUpdated& Conf::getVariableUpdatedSignal() noexcept {
    return pimpl->variableUpdated;
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
