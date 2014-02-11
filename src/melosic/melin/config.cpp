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
namespace { namespace mpl = boost::mpl; }
#include <shared_mutex>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/format.hpp>
namespace { using boost::format; }

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

struct Manager::impl {
    typedef std::mutex Mutex;
    using lock_guard = std::lock_guard<Mutex>;
    using unique_lock = std::unique_lock<Mutex>;

    impl(fs::path p) : conf(Conf("root")) {
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

    Conf ConfFromJson(json::Value& val, KeyType name) {
        assert(val.IsObject());
        auto members = boost::make_iterator_range(val.MemberBegin(), val.MemberEnd());
        Conf c(name);
        for(auto& member : members) {
            KeyType name_str(member.name.GetString());
            if(member.value.IsObject()) {
                TRACE_LOG(logject) << "adding child conf from json: " << name_str;
                c.putChild(ConfFromJson(member.value, std::move(name_str)));
            }
            else {
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
            loaded(std::ref(conf));
            saveConfig();
            return;
        }

        fs::ifstream ifs(confPath);
        assert(ifs.good());

        ifs.seekg(0, std::ios::end);
        auto confstring = std::vector<char>{};
        confstring.resize(static_cast<std::streamoff>(ifs.tellg())+1);
        ifs.seekg(0, std::ios::beg);
        ifs.read(confstring.data(), confstring.size()-1);
        ifs.close();
        confstring.back() = 0;

        json::Document rootjson;
        rootjson.ParseInsitu<0>(confstring.data());

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
        }
        catch(boost::bad_get& e) {
            return {};
        }
    }

    json::Value JsonFromConf(const Conf& c) {
        json::Value obj(json::kObjectType);
        c.iterateNodes([&] (const std::pair<KeyType, VarType>& pair) {
            TRACE_LOG(logject) << "adding node to json: " << pair.first;
            auto val = JsonFromVar(pair.second);
            if(val.IsNull()) {
                TRACE_LOG(logject) << "node is null. skipping...";
                return;
            }
            obj.AddMember(pair.first.c_str(), std::move(val), poolAlloc);
        });
        c.iterateChildren([&] (const Conf& childconf) {
            TRACE_LOG(logject) << "adding conf to json: " << childconf.getName();
            auto child = JsonFromConf(childconf);
            if(std::distance(child.MemberBegin(), child.MemberEnd()) == 0) {
                TRACE_LOG(logject) << "conf child is empty. skipping...";
                return;
            }
            obj.AddMember(childconf.getName().c_str(), std::move(child), poolAlloc);
        });
        return obj;
    }

    void saveConfig() {
        TRACE_LOG(logject) << "saving config as json";

        lock_guard l(mu);
        assert(!confPath.empty());

        auto c = conf.synchronize();
        json::Document rootjson(JsonFromConf(*c));
        json::StringBuffer strbuf;
        json::PrettyWriter<json::StringBuffer> writer(strbuf);
        rootjson.Accept(writer);

        std::string stringified(strbuf.GetString());
        TRACE_LOG(logject) << "JSON: " << stringified;
        fs::ofstream ofs(confPath);
        assert(ofs.good());
        ofs << stringified << std::endl;
    }

    boost::synchronized_value<Conf>& getConfigRoot() {
        return conf;
    }

    Mutex mu;
    json::Value::AllocatorType poolAlloc;
    fs::path confPath;
    boost::synchronized_value<Conf> conf;
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

boost::synchronized_value<Conf>& Manager::getConfigRoot() {
    return pimpl->getConfigRoot();
}

Signals::Config::Loaded& Manager::getLoadedSignal() const {
    return pimpl->loaded;
}

struct Conf::impl {
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

    threadsafe_list<Conf> children;
    threadsafe_list<std::pair<std::add_const<KeyType>::type, VarType>> nodes;
    KeyType name;
    Conf::DefaultFunc resetDefault;

    void merge(impl& c);

    void addDefaultFunc(Conf::DefaultFunc func);
    Conf resetToDefault();
};

void Conf::impl::addDefaultFunc(Conf::DefaultFunc func) {
    resetDefault = func;
}

Conf Conf::impl::resetToDefault() {
    if(!resetDefault)
        return Conf();
    return std::move(resetDefault());
}

Conf::Conf() : Conf(""s) {}

Conf::Conf(KeyType name) :
    pimpl(std::make_unique<impl>(std::move(name)))
{}

Conf::Conf(Conf&& b) {
    using std::swap;
    swap(*this, b);
}

Conf::~Conf() {}

Conf::Conf(const Conf& b) {
    pimpl = std::make_unique<impl>(*b.pimpl);
}

Conf& Conf::operator=(Conf b) {
    using std::swap;
    swap(*this, b);
    return *this;
}

typename std::add_const<KeyType>::type& Conf::getName() const noexcept {
    return pimpl->name;
}

std::shared_ptr<std::pair<KeyType, VarType>> Conf::getNode(std::add_const<KeyType>::type& key) {
    TRACE_LOG(logject) << "Getting node: " << key;
    return pimpl->nodes.find_first_if([&] (const std::pair<KeyType, VarType>& pair) {
        return pair.first == key;
    });
}

std::shared_ptr<const std::pair<KeyType, VarType>> Conf::getNode(std::add_const<KeyType>::type& key) const {
    TRACE_LOG(logject) << "Getting node: " << key;
    return pimpl->nodes.find_first_if([&] (const std::pair<KeyType, VarType>& pair) {
        return pair.first == key;
    });
}

std::shared_ptr<Conf::ChildType> Conf::getChild(std::add_const<KeyType>::type& key) {
    TRACE_LOG(logject) << "Getting child: " << key;
    return pimpl->children.find_first_if([&] (const Conf& c) {
        return c.getName() == key;
    });
}

std::shared_ptr<const Conf::ChildType> Conf::getChild(std::add_const<KeyType>::type& key) const {
    TRACE_LOG(logject) << "Getting child: " << key;
    return pimpl->children.find_first_if([&] (const Conf& c) {
        return c.getName() == key;
    });
}

void Conf::putChild(Conf child) {
    using std::swap;
    auto c = getChild(child.getName());
    if(c)
        swap(*c, child);
    else
        pimpl->children.push_front(std::move(child));
}

void Conf::putNode(KeyType key, VarType value) {
    using std::swap;
    auto c = getNode(key);
    std::pair<KeyType, VarType> pair(std::move(key), std::move(value));
    if(c)
        swap(c->second, pair.second);
    else
        pimpl->nodes.push_front(std::move(pair));
}

void Conf::removeChild(std::add_const<KeyType>::type& key) {
    return pimpl->children.remove_if([&] (const Conf& c) {
        return c.getName() == key;
    });
}

void Conf::removeNode(std::add_const<KeyType>::type& key) {
    return pimpl->nodes.remove_if([&] (const std::pair<std::add_const<KeyType>::type, VarType>& pair) {
        return pair.first == key;
    });
}

uint32_t Conf::childCount() const noexcept {
    return pimpl->children.size();
}
uint32_t Conf::nodeCount() const noexcept {
    return pimpl->nodes.size();
}

void Conf::iterateChildren(std::function<void(const ChildType&)> fun) const {
    pimpl->children.for_each(std::move(fun));
}

void Conf::iterateChildren(std::function<void(ChildType&)> fun) {
    pimpl->children.for_each(std::move(fun));
}

void Conf::iterateNodes(std::function<void(const std::pair<KeyType, VarType>&)> fun) const {
    pimpl->nodes.for_each(std::move(fun));
}

void Conf::iterateNodes(std::function<void(std::pair<KeyType, VarType>&)> fun) {
    pimpl->nodes.for_each(std::move(fun));
}

void Conf::merge(const Conf& c) {
    c.iterateNodes([this] (const std::pair<KeyType, VarType>& pair) {
        auto tmp = getNode(pair.first);
        if(!tmp) {
            putNode(pair.first, pair.second);
            tmp = getNode(pair.first);
        }
        assert(tmp);
    });
    c.iterateChildren([this] (const Conf& child) {
        auto tmp = getChild(child.getName());
        if(!tmp) {
            putChild(child);
            tmp = getChild(child.getName());
        }
        assert(tmp);
        tmp->merge(child);
    });
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
} // namespace Melosic
