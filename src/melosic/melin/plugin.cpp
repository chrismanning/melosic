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

#include <map>
#include <memory>
#include <string>
#include <functional>
#include <regex>
#include <mutex>

#include <boost/range/algorithm/transform.hpp>
#include <boost/range/adaptor/map.hpp>
using namespace boost::adaptors;
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/variant.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/scope_exit.hpp>
#include <boost/dll.hpp>

#include <melosic/melin/kernel.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/error.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/configvar.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/common/bit_flag_iterator.hpp>
#include <melosic/melin/decoder.hpp>

#include "plugin.hpp"

namespace Melosic {
namespace Plugin {

struct PluginsLoaded : Signals::Signal<Signals::Plugin::PluginsLoaded> {};

Logger::Logger logject{logging::keywords::channel = "Plugin::Manager"};

struct Plugin : std::enable_shared_from_this<Plugin> {
  public:
    Plugin(const boost::filesystem::path& filename) : handle(filename) {
        auto plugin_info_fun = handle.get_alias<Info()>("plugin_info");
        info = plugin_info_fun();

        if(info.APIVersion != expectedAPIVersion()) {
            BOOST_THROW_EXCEPTION(PluginVersionMismatchException() << ErrorTag::FilePath(handle.location())
                                                                   << ErrorTag::Plugin::Info(info));
        }
    }

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    Plugin(Plugin&&) noexcept = default;

    const Info& getInfo() const {
        return info;
    }

    void init(const Core::Kernel& kernel) {
        for(auto flag : make_flag_range(info.type)) {
            switch(flag) {
                case Type::decoder: {
                    std::shared_ptr<Decoder::Manager> decman = kernel.getDecoderManager();
                    decman->add_provider(std::shared_ptr<Decoder::provider>(
                        shared_from_this(), handle.get_alias<Decoder::provider*()>("decoder_provider")()));
                    break;
                }
                case Type::encoder:
                case Type::outputDevice:
                case Type::inputDevice:
                case Type::utility:
                case Type::service:
                case Type::gui:
                    break;
            };
        }
    }

  private:
    boost::dll::shared_library handle;
    Info info;
};

struct Manager::impl {
    using mutex = std::mutex;
    using lock_guard = std::lock_guard<mutex>;
    using unique_lock = std::unique_lock<mutex>;
    using strict_lock = boost::strict_lock<mutex>;

    impl(const std::shared_ptr<Config::Manager>& confman) : confman(confman) {
        searchPaths.push_back(fs::current_path().parent_path() / "lib");
        conf.putNode("search paths", searchPaths);
        confman->getLoadedSignal().connect(&impl::loadedSlot, this);
    }

    void loadedSlot(boost::synchronized_value<Config::Conf>& base) {
        TRACE_LOG(logject) << "Plugin conf loaded";

        auto c = base->createChild("Plugins", conf);

        c->merge(conf);
        c->setDefault(conf);
        c->iterateNodes([this](const std::string& key, auto&& var) { variableUpdateSlot(key, var); });
        m_signal_connections.emplace_back(c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this));
    }

    void variableUpdateSlot(const Config::Conf::node_key_type& key, const Config::VarType& val) {
        using std::get;
        TRACE_LOG(logject) << "Config: variable updated: " << key;
        try {
            if(key == "search paths") {
                strict_lock l(mu);
                searchPaths.clear();
                for(auto& path : get<std::vector<Config::VarType>>(val))
                    searchPaths.emplace_back(get<std::string>(path));
                if(searchPaths.empty()) {
                    auto sp = conf.getNode("search paths");
                    if(sp)
                        for(auto& path : get<std::vector<Config::VarType>>(*sp))
                            searchPaths.emplace_back(get<std::string>(path));
                }
            } else if(key == "blacklist") {
                strict_lock l(mu);
                blackList.clear();
                for(auto& path : get<std::vector<Config::VarType>>(val))
                    blackList.push_back(get<std::string>(path));
            } else
                ERROR_LOG(logject) << "Config: Unknown key: " << key;
        } catch(boost::bad_get&) {
            ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        }
    }

    void loadPlugin(fs::path filepath, strict_lock& l) {
        filepath = search_absolute_path(filepath, l);
        if(!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
            BOOST_THROW_EXCEPTION(PluginException() << ErrorTag::FilePath(filepath)
                                                    << ErrorTag::Plugin::Msg("no such file"));
        }

        auto filename = filepath.filename().string();

        if(loadedPlugins.find(filename) != loadedPlugins.end())
            BOOST_THROW_EXCEPTION(PluginException() << ErrorTag::FilePath(filename)
                                                    << ErrorTag::Plugin::Msg("plugin already loaded"));

        auto loaded = std::make_shared<Plugin>(filepath);
        LOG(logject) << "Plugin loaded: " << loaded->getInfo();
        loadedPlugins.emplace(filename, std::move(loaded));
    }

    fs::path search_absolute_path(fs::path filepath, strict_lock&) {
        if(!filepath.is_absolute()) {
            TRACE_LOG(logject) << "plugin path relative";
            for(const fs::path& searchpath : searchPaths) {
                TRACE_LOG(logject) << "Plugin search path: " << searchpath;
                auto p = fs::absolute(filepath, searchpath);
                if(fs::exists(p)) {
                    return p;
                }
            }
        }
        return filepath;
    }

    void loadPlugins(Core::Kernel& kernel) {
        BOOST_SCOPE_EXIT_ALL(&) {
            pluginsLoaded(getPlugins());
        };
        strict_lock l(mu);

        TRACE_LOG(logject) << "Loading plugins...";
        std::regex filter{boost::algorithm::join(blackList, "|")};
        for(const fs::path& dir : searchPaths) {
            if(!fs::exists(dir))
                continue;
            TRACE_LOG(logject) << "In search path: " << dir;

            for(const fs::path& file : boost::make_iterator_range(fs::recursive_directory_iterator(dir), {})) {
                try {
                    if(std::regex_match(file.string(), filter)) {
                        WARN_LOG(logject) << file << " is blacklisted";
                        continue;
                    }
                    if(fs::is_regular_file(file) && file.extension() == ".melin")
                        loadPlugin(file, l);
                } catch(...) {
                    ERROR_LOG(logject) << "Plugin " << file << " not loaded";
                    ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
                }
            }
        }
        initialise(kernel);
    }

    std::vector<Info> getPlugins() {
        strict_lock l(mu);
        std::vector<Info> vec;
        boost::transform(loadedPlugins | map_values, std::back_inserter(vec), [](auto& p) { return p->getInfo(); });
        return vec;
    }

    void initialise(Core::Kernel& kernel) {
        for(auto& plugin : loadedPlugins | map_values) {
            TRACE_LOG(logject) << "Initialising plugin " << plugin->getInfo().name;
            plugin->init(kernel);
        }
    }

    std::shared_ptr<Config::Manager> confman;
    Config::Conf conf{"Plugins"};
    std::map<std::string, std::shared_ptr<Plugin>> loadedPlugins;
    std::vector<fs::path> searchPaths;
    std::vector<std::string> blackList;
    friend class Manager;
    friend struct RegisterFuncsInserter;
    PluginsLoaded pluginsLoaded;
    std::vector<Signals::ScopedConnection> m_signal_connections;

    mutex mu;
};

Manager::Manager(const std::shared_ptr<Config::Manager>& confman) : pimpl(new impl(confman)) {
}

Manager::~Manager() {
}

void Manager::loadPlugins(Core::Kernel& kernel) {
    pimpl->loadPlugins(kernel);
}

std::vector<Info> Manager::getPlugins() const {
    return pimpl->getPlugins();
}

Signals::Plugin::PluginsLoaded& Manager::getPluginsLoadedSignal() const {
    return pimpl->pluginsLoaded;
}

} // namespace Plugin
} // namespace Melosic
