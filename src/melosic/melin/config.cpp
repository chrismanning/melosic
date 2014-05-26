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

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;
#include <boost/mpl/index_of.hpp>
namespace {
namespace mpl = boost::mpl;
}
#include <shared_mutex>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/format.hpp>
namespace {
using boost::format;
}

#include <melosic/common/configvar.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/common/threadsafe_list.hpp>

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

using mutex = std::recursive_timed_mutex;
using lock_guard = std::lock_guard<mutex>;
using unique_lock = std::unique_lock<mutex>;

struct Manager::impl {
    impl(fs::path p) : conf(Conf("root")) {
        if(p.empty())
            BOOST_THROW_EXCEPTION(Exception());

        confPath = std::move(Directories::configHome() / "melosic" / p.filename());

        if(!fs::exists(confPath.parent_path()))
            fs::create_directories(confPath.parent_path());
        TRACE_LOG(logject) << "Conf file path: " << confPath;
    }

    ~impl() {
        unique_lock l(mu, 100ms);
        assert(l);
    }

    VarType VarFromJson(json::Value& val) {
        if(val.IsString())
            return VarType(std::string(val.GetString()));
        else if(val.IsBool())
            return VarType(val.GetBool());
        else if(val.IsInt())
            return VarType(val.GetInt());
        else if(val.IsUint())
            return VarType(val.GetUint());
        else if(val.IsInt64())
            return VarType(val.GetInt64());
        else if(val.IsUint64())
            return VarType(val.GetUint64());
        else if(val.IsDouble())
            return VarType(val.GetDouble());
        else if(val.IsArray()) {
            std::vector<VarType> vec;
            vec.reserve(val.Size());
            for(auto& v : boost::make_iterator_range(val.Begin(), val.End()))
                vec.push_back(VarFromJson(v));
            return VarType(std::move(vec));
        }
        assert(false);
    }

    Conf ConfFromJson(json::Value& val, const Config::Conf::node_key_type& name) {
        assert(val.IsObject());
        auto members = boost::make_iterator_range(val.MemberBegin(), val.MemberEnd());
        Conf c(name);
        for(auto& member : members) {
            Config::Conf::node_key_type name_str(member.name.GetString());
            if(member.value.IsObject()) {
                TRACE_LOG(logject) << "adding child conf from json: " << name_str;
                c.putChild(ConfFromJson(member.value, std::move(name_str)));
            } else {
                TRACE_LOG(logject) << "adding node from json: " << name_str;
                c.putNode(std::move(name_str), VarFromJson(member.value));
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
            auto locked_conf = getConfigRoot();
            loaded(std::ref(locked_conf));
            saveConfig();
            return;
        }

        fs::ifstream ifs(confPath);
        assert(ifs.good());

        ifs.seekg(0, std::ios::end);
        auto confstring = std::vector<char>{};
        confstring.resize(static_cast<std::streamoff>(ifs.tellg()) + 1);
        ifs.seekg(0, std::ios::beg);
        ifs.read(confstring.data(), confstring.size() - 1);
        ifs.close();
        confstring.back() = 0;

        json::Document rootjson;
        rootjson.ParseInsitu<0>(confstring.data());

        if(!rootjson.HasParseError()) {
            Conf tmp_conf = std::move(ConfFromJson(rootjson, "root"s));

            using std::swap;
            swap(conf, tmp_conf);
        } else
            ERROR_LOG(logject) << "JSON parse error: " << rootjson.GetParseError();

        l.unlock();
        auto locked_conf = getConfigRoot();
        loaded(std::ref(locked_conf));
        saveConfig();
    }

    template <typename T> static constexpr int TypeIndex = mpl::index_of<VarType::types, T>::type::value;
    json::Value JsonFromVar(const VarType& var) {
        try {
            switch(var.which()) {
                case TypeIndex<std::string>: {
                    const auto& tmp = boost::get<std::string>(var);
                    return json::Value(tmp.c_str(), tmp.size(), poolAlloc);
                }
                case TypeIndex<bool>:
                    return json::Value(boost::get<bool>(var));
                case TypeIndex<int32_t>:
                    return json::Value(boost::get<int32_t>(var));
                case TypeIndex<uint32_t>:
                    return json::Value(boost::get<uint32_t>(var));
                case TypeIndex<int64_t>:
                    return json::Value(boost::get<int64_t>(var));
                case TypeIndex<uint64_t>:
                    return json::Value(boost::get<uint64_t>(var));
                case TypeIndex<double>:
                    return json::Value(boost::get<double>(var));
                case TypeIndex<std::vector<VarType>>: {
                    auto& vec = boost::get<std::vector<VarType>>(var);
                    json::Value val(json::kArrayType);
                    val.Reserve(vec.size(), poolAlloc);
                    for(auto& v : vec)
                        val.PushBack(JsonFromVar(v), poolAlloc);
                    return std::move(val);
                }
            }
            assert(false);
        } catch(boost::bad_get& e) {
            return {};
        }
    }

    json::Value JsonFromConf(const Conf& c) {
        json::Value obj(json::kObjectType);
        c.iterateNodes([&](const std::string& key, auto&& locked_var) {
            TRACE_LOG(logject) << "adding node to json: " << key;
            auto val = JsonFromVar(locked_var);
            if(val.IsNull()) {
                TRACE_LOG(logject) << "node is null. skipping...";
                return;
            }
            obj.AddMember(key.data(), std::move(val), poolAlloc);
        });
        c.iterateChildren([&](auto&& childconf) {
            TRACE_LOG(logject) << "adding conf to json: " << childconf->name();
            auto child = JsonFromConf(*childconf);
            if(std::distance(child.MemberBegin(), child.MemberEnd()) == 0) {
                TRACE_LOG(logject) << "conf child is empty. skipping...";
                return;
            }
            obj.AddMember(childconf->name().data(), std::move(child), poolAlloc);
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

    boost::unique_lock_ptr<Conf, mutex> getConfigRoot() {
        return {conf, mu};
    }

    mutex mu;
    json::Value::AllocatorType poolAlloc;
    fs::path confPath;
    Conf conf;
    Loaded loaded;
};

Manager::Manager(fs::path p) : pimpl(new impl(std::move(p))) {}

Manager::~Manager() {}

void Manager::loadConfig() { pimpl->loadConfig(); }

void Manager::saveConfig() { pimpl->saveConfig(); }

boost::unique_lock_ptr<Conf, mutex> Manager::getConfigRoot() { return pimpl->getConfigRoot(); }

Signals::Config::Loaded& Manager::getLoadedSignal() const { return pimpl->loaded; }

struct ConfCompare {
    using is_transparent = std::true_type;
    bool operator()(const Conf& a, const Conf& b) const { return a < b; }
    bool operator()(const boost::intrusive_ptr<Conf>& a, const boost::intrusive_ptr<Conf>& b) const {
        assert(a);
        assert(b);
        return (*this)(*a, *b);
    }
    bool operator()(const boost::intrusive_ptr<Conf>& a, const Conf& b) const {
        assert(a);
        return (*this)(*a, b);
    }
    bool operator()(const Conf& a, const boost::intrusive_ptr<Conf>& b) const {
        assert(b);
        return (*this)(a, *b);
    }
    bool operator()(const Conf::child_key_type& a, const boost::intrusive_ptr<Conf>& b) const {
        assert(b);
        return (*this)(a, *b);
    }
    bool operator()(const boost::intrusive_ptr<Conf>& a, const Conf::child_key_type& b) const {
        assert(a);
        return (*this)(*a, b);
    }
    bool operator()(const Conf::child_key_type& a, const Conf& b) const { return a < b.name(); }
    bool operator()(const Conf& a, const Conf::child_key_type& b) const { return a.name() < b; }
};

struct Conf::impl {
    impl(std::string name) : name(std::move(name)) {}

    impl(const impl& b) : children(b.children), nodes(b.nodes), name(b.name), m_default(b.m_default) {}

    impl(impl&&) = default;

    void merge(impl& c);

    void setDefault(Conf);
    Conf resetToDefault();

    std::set<Conf::child_value_type, ConfCompare> children;
    std::map<std::string, Conf::node_mapped_type> nodes;

    std::string name;
    std::experimental::optional<Conf> m_default;

    mutex mu;

    struct VariableUpdated : Signals::Signal<Signals::Config::VariableUpdated> {
        using Super::Signal;
    } variableUpdated;
};

void Conf::impl::setDefault(Conf def) { m_default = def; }

Conf Conf::impl::resetToDefault() {
    if(!m_default)
        return Conf(name);
    return *m_default;
}

Conf::Conf() : Conf(""s) {}

Conf::Conf(std::string name) : pimpl(std::make_unique<impl>(std::move(name))) {}

Conf::Conf(Conf&& b) : pimpl(std::move(b.pimpl)) {}

Conf::~Conf() {}

Conf::Conf(const Conf& b) {
    assert(b.pimpl);
    unique_lock l(b.pimpl->mu);
    pimpl = std::make_unique<impl>(*b.pimpl);
}

Conf& Conf::operator=(Conf&& b) & {
    pimpl = std::move(b.pimpl);
    return *this;
}

Conf& Conf::operator=(const Conf& b) & {
    *this = Conf(b);
    return *this;
}

bool Conf::operator<(const Conf& b) const { return pimpl->name < b.pimpl->name; }

bool Conf::operator==(const Conf& b) const {
    return pimpl->name == b.pimpl->name && pimpl->children == b.pimpl->children && pimpl->nodes == b.pimpl->nodes;
}

void Conf::swap(Conf& b) noexcept(noexcept(pimpl.swap(b.pimpl))) { pimpl.swap(b.pimpl); }

const std::string& Conf::name() const { return pimpl->name; }

auto Conf::getChild(const child_key_type& key) -> child_value_type {
    TRACE_LOG(logject) << "Getting child: " << key;
    unique_lock l(pimpl->mu);

    auto it = pimpl->children.find(key);
    if(it == pimpl->children.end())
        return {};
    return *it;
}

auto Conf::getChild(const child_key_type& key) const -> child_const_value_type {
    TRACE_LOG(logject) << "Getting child: " << key;
    unique_lock l(pimpl->mu);

    auto it = pimpl->children.find(key);
    if(it == pimpl->children.end())
        return {};
    return boost::const_pointer_cast<const Conf>(*it);
}

auto Conf::createChild(const child_key_type& key) -> child_value_type {
    return createChild(key, Conf{key});
}

auto Conf::createChild(const child_key_type& key, const child_value_type& def) -> child_value_type {
    assert(def);
    return createChild(key, child_value_type(def));
}

auto Conf::createChild(const child_key_type& key, child_value_type&& def) -> child_value_type {
    assert(def);
    unique_lock l(pimpl->mu);

    auto it = pimpl->children.find(key);
    if(it == pimpl->children.end())
        return putChild(std::move(def));
    return *it;
}

auto Conf::createChild(const child_key_type& key, const Conf& def) -> child_value_type {
    return createChild(key, Conf{def});
}

auto Conf::createChild(const child_key_type& key, Conf&& def) -> child_value_type {
    return createChild(key, new Conf{std::move(def)});
}

auto Conf::putChild(const child_value_type& child) -> child_value_type {
    return putChild(child_value_type{child});
}

auto Conf::putChild(child_value_type&& child) -> child_value_type {
    assert(child);
    using std::get;
    TRACE_LOG(logject) << "Putting child: " << child->name();
    unique_lock l(pimpl->mu);

    auto& children = pimpl->children;
    auto ret = children.insert(std::move(child));
    if(get<1>(ret))
        return *get<0>(ret);

    TRACE_LOG(logject) << "Child \"" << child->name() << "\" already exists; replacing.";
    return *children.insert(children.erase(get<0>(ret)), std::move(child));
}

auto Conf::putChild(const Conf& child) -> child_value_type {
    return putChild(Conf{child});
}

auto Conf::putChild(Conf&& child) -> child_value_type {
    return putChild(new Conf{std::move(child)});
}

void Conf::removeChild(const child_key_type& key) {
    TRACE_LOG(logject) << "Removing child: " << key;
    unique_lock l(pimpl->mu);

    auto& children = pimpl->children;
    auto it = children.find(key);
    if(it != children.end())
        children.erase(it);
    else
        TRACE_LOG(logject) << "Cannot removing non-existant child: " << key;
}

auto Conf::getNode(const node_key_type& key) const -> std::experimental::optional<node_mapped_type> {
    TRACE_LOG(logject) << "Getting node: " << key;
    unique_lock l(pimpl->mu);

    auto it = pimpl->nodes.find(key);
    if(it == pimpl->nodes.end())
        return std::experimental::nullopt;
    return it->second;
}

auto Conf::createNode(const Conf::node_key_type& key) -> node_mapped_type {
    return createNode(key, node_mapped_type{});
}

auto Conf::createNode(const node_key_type& key, const node_mapped_type& def) -> node_mapped_type {
    return createNode(key, node_mapped_type{def});
}

auto Conf::createNode(const node_key_type& key, node_mapped_type&& def) -> node_mapped_type {
    using std::get;
    unique_lock l(pimpl->mu);

    auto it = pimpl->nodes.find(key);
    if(it == pimpl->nodes.end()) {
        auto ret = pimpl->nodes.emplace(std::make_pair(key, std::move(def)));
        assert(get<1>(ret));
        it = get<0>(ret);
    }
    return it->second;
}

void Conf::putNode(const node_key_type& key, const node_mapped_type& value) {
    putNode(key, node_mapped_type{value});
}

void Conf::putNode(const node_key_type& key, node_mapped_type&& value) {
    using std::get;
    TRACE_LOG(logject) << "Putting node: " << key;
    unique_lock l(pimpl->mu);

    node_mapped_type node;

    auto ret = pimpl->nodes.emplace(std::make_pair(key, std::move(value)));
    if(get<1>(ret))
        node = std::get<0>(ret)->second;
    else {
        TRACE_LOG(logject) << "Node \"" << key << "\" already exists; replacing.";
        node = pimpl->nodes.insert(pimpl->nodes.erase(get<0>(ret)), std::make_pair(key, std::move(value)))->second;
    }
    pimpl->variableUpdated(std::cref(key), std::cref(node));
}

void Conf::removeNode(const node_key_type& key) {
    TRACE_LOG(logject) << "Removing node: " << key;
    unique_lock l(pimpl->mu);

    pimpl->nodes.erase(key);
}

uint32_t Conf::childCount() const noexcept { return pimpl->children.size(); }
uint32_t Conf::nodeCount() const noexcept { return pimpl->nodes.size(); }

void Conf::iterateChildren(std::function<void(child_const_value_type)> fun) const {
    using std::get;
    unique_lock l(pimpl->mu);

    for(auto val : pimpl->children) {
        assert(val);
        fun(val);
    }
}

void Conf::iterateNodes(std::function<void(const node_key_type&, const node_mapped_type&)> fun) const {
    using std::get;
    unique_lock l(pimpl->mu);

    for(const auto& pair : pimpl->nodes)
        fun(get<0>(pair), get<1>(pair));
}

void Conf::merge(const Conf& c) {
    unique_lock l1(pimpl->mu, std::defer_lock), l2(c.pimpl->mu, std::defer_lock);
    std::lock(l1, l2);

    pimpl->nodes.insert(c.pimpl->nodes.begin(), c.pimpl->nodes.end());

    c.iterateChildren([this](auto&& child) {
        auto tmp = createChild(child->name(), *child);
        tmp->merge(*child);
    });
}

void Conf::setDefault(Conf def) { pimpl->setDefault(std::move(def)); }
void Conf::resetToDefault() { *this = pimpl->resetToDefault(); }

Signals::Config::VariableUpdated& Conf::getVariableUpdatedSignal() noexcept { return pimpl->variableUpdated; }

void swap(Conf& a, Conf& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

} // namespace Config
} // namespace Melosic
