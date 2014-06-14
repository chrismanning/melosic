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

#include <boost/config.hpp>

#if defined(BOOST_WINDOWS)

#include <windows.h>
#define DLHandle HMODULE
#define DLOpen(a) LoadLibrary(a)
#define DLClose FreeLibrary
#define DLGetSym GetProcAddress
inline const char * DLError() {
    char * str;
    auto err = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPSTR)&str,
                  0,
                  NULL);
    return str;
}

#else // non-windows platforms

#include <dlfcn.h>
#define DLHandle void*
#define DLOpen(a) ::dlopen(a, RTLD_NOW)
#define DLClose ::dlclose
#define DLGetSym ::dlsym
#define DLError ::dlerror

#endif

#include <map>
#include <memory>
#include <string>
#include <list>
#include <functional>
#include <regex>

#include <boost/range.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>
using namespace boost::adaptors;
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/variant.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/scope_exit.hpp>

#include <melosic/melin/kernel.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/error.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/configvar.hpp>
#include <melosic/common/signal.hpp>

#include "plugin.hpp"

namespace Melosic {
namespace Plugin {

struct PluginsLoaded : Signals::Signal<Signals::Plugin::PluginsLoaded> {};

class Plugin {
public:
    Plugin(const boost::filesystem::path& filename, Core::Kernel& kernel)
        : pluginPath(absolute(filename)),
          kernel(kernel)
    {
        handle = DLOpen(pluginPath.string().c_str());

        if(handle == nullptr) {
            BOOST_THROW_EXCEPTION(PluginException() <<
                                  ErrorTag::FilePath(pluginPath) <<
                                  ErrorTag::Plugin::Msg(DLError()));
        }

        getFunction<registerPlugin_F>("registerPlugin")(&info, RegisterFuncsInserter(kernel, regFuns));
        if(info.APIVersion != expectedAPIVersion()) {
            BOOST_THROW_EXCEPTION(PluginVersionMismatchException() <<
                                  ErrorTag::FilePath(pluginPath) <<
                                  ErrorTag::Plugin::Info(info));
        }
        destroyPlugin_ = getFunction<destroyPlugin_F>("destroyPlugin");
    }

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    Plugin(Plugin&& b)
        : pluginPath(b.pluginPath),
          destroyPlugin_(b.destroyPlugin_),
          regFuns(b.regFuns),
          handle(b.handle),
          kernel(b.kernel),
          info(b.info)
    {
        b.handle = nullptr;
    }

    ~Plugin() {
        if(handle) {
            regFuns.clear();
            if(destroyPlugin_) {
                destroyPlugin_();
            }
            DLClose(handle);
        }
    }

    const Info& getInfo() const {
        return info;
    }

    void init() {
        for(auto& fun : regFuns) {
            fun();
        }
    }

private:
    template <typename T>
    std::function<T> getFunction(const std::string& symbolName) {
#ifdef _POSIX_VERSION
        DLError(); //clear err
#endif
        auto sym = DLGetSym(handle, symbolName.c_str());
        auto err = DLError();
#if defined(_POSIX_VERSION)
        if(err != nullptr) {
#elif defined(_WIN32)
        if(sym == nullptr) {
#endif
            BOOST_THROW_EXCEPTION(PluginSymbolNotFoundException() <<
                                  ErrorTag::FilePath(pluginPath) <<
                                  ErrorTag::Plugin::Symbol(symbolName)<<
                                  ErrorTag::Plugin::Msg(err));
        }
        return reinterpret_cast<T*>(sym);
    }

    boost::filesystem::path pluginPath;
    destroyPlugin_T destroyPlugin_;
    std::list<std::function<void()>> regFuns;
    DLHandle handle;
    Core::Kernel& kernel;
    Info info;
};

class Manager::impl {
public:
    impl(Config::Manager& confman) : confman(confman) {
        addSearchPath(fs::current_path().parent_path()/"lib");
        conf.putNode("search paths", searchPaths);
        confman.getLoadedSignal().connect(&impl::loadedSlot, this);
    }

    void loadedSlot(boost::synchronized_value<Config::Conf>& base) {
        TRACE_LOG(logject) << "Plugin conf loaded";

        auto c = base->createChild("Plugins", conf);

        c->merge(conf);
        c->setDefault(conf);
        c->iterateNodes([this] (const std::string& key, auto&& var) {
            variableUpdateSlot(key, var);
        });
        m_signal_connections.emplace_back(c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this));
    }

    void variableUpdateSlot(const Config::Conf::node_key_type& key, const Config::VarType& val) {
        using std::get;
        TRACE_LOG(logject) << "Config: variable updated: " << key;
        try {
            if(key == "search paths") {
                searchPaths.clear();
                for(auto& path : get<std::vector<Config::VarType>>(val))
                    searchPaths.emplace_back(get<std::string>(path));
                if(searchPaths.empty()) {
                    auto sp = conf.getNode("search paths");
                    if(sp)
                        for(auto& path : get<std::vector<Config::VarType>>(*sp))
                            searchPaths.emplace_back(get<std::string>(path));
                }
            }
            else if(key == "blacklist") {
                blackList.clear();
                for(auto& path : get<std::vector<Config::VarType>>(val))
                    blackList.push_back(get<std::string>(path));
            }
            else
                ERROR_LOG(logject) << "Config: Unknown key: " << key;
        }
        catch(boost::bad_get&) {
            ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        }
    }

    void addSearchPath(const fs::path& pluginpath) {
        if(fs::exists(pluginpath) && fs::is_directory(pluginpath) && pluginpath.is_absolute())
            searchPaths.push_back(pluginpath);
        else
            ERROR_LOG(logject) << pluginpath << " not a directory";
    }

    void loadPlugin(fs::path filepath, Core::Kernel& kernel) {
        if(!filepath.is_absolute()) {
            TRACE_LOG(logject) << "plugin path relative";
            for(const fs::path& searchpath : searchPaths) {
                TRACE_LOG(logject) << "Plugin search path: " << searchpath;
                auto p(fs::absolute(filepath, searchpath));
                if(fs::exists(p)) {
                    filepath = p;
                    break;
                }
            }
        }

        auto filename = filepath.filename().string();

        if(loadedPlugins.find(filename) != loadedPlugins.end())
            BOOST_THROW_EXCEPTION(PluginSymbolNotFoundException() <<
                                  ErrorTag::FilePath(filename) <<
                                  ErrorTag::Plugin::Msg("plugin already loaded"));

        auto info = loadedPlugins.emplace(filename, Plugin(filepath, kernel)).first->second.getInfo();
        LOG(logject) << "Plugin loaded: " << info;
    }

    void loadPlugins(Core::Kernel& kernel) {
        BOOST_SCOPE_EXIT_ALL(&) {
            pluginsLoaded(getPlugins());
        };

        TRACE_LOG(logject) << "Loading plugins...";
        std::regex filter{boost::algorithm::join(blackList, "|")};
        for(const fs::path& dir : searchPaths) {
            if(!fs::exists(dir))
                continue;
            TRACE_LOG(logject) << "In search path: " << dir;

            for(const fs::path& file : boost::make_iterator_range(fs::recursive_directory_iterator(dir), {})) {
                try {
                    if(std::regex_match(file.string(), filter)) {
                        WARN_LOG(logject) << file << " matches blacklist";
                        continue;
                    }
                    if(fs::is_regular_file(file) && file.extension() == ".melin")
                        loadPlugin(file, kernel);
                }
                catch(...) {
                    ERROR_LOG(logject) << "Plugin " << file << " not loaded";
                    ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
                }
            }
        }
        initialise();
    }

    ForwardRange<const Info> getPlugins() {
        std::function<Info(Plugin&)> fun([](Plugin& p) {return p.getInfo();});
        return loadedPlugins | map_values | transformed(fun);
    }

    void initialise() {
        for(Plugin& plugin : loadedPlugins | map_values) {
            TRACE_LOG(logject) << "Initialising plugin " << plugin.getInfo().name;
            plugin.init();
        }
    }

private:
    Config::Manager& confman;
    Config::Conf conf{"Plugins"};
    std::map<std::string, Plugin> loadedPlugins;
    Logger::Logger logject{logging::keywords::channel = "Plugin::Manager"};
    std::list<fs::path> searchPaths;
    std::list<std::string> blackList;
    friend class Manager;
    friend struct RegisterFuncsInserter;
    PluginsLoaded pluginsLoaded;
    std::vector<Signals::ScopedConnection> m_signal_connections;
};

Manager::Manager(Config::Manager& confman) : pimpl(new impl(confman)) {}

Manager::~Manager() {}

void Manager::loadPlugins(Core::Kernel& kernel) {
    pimpl->loadPlugins(kernel);
}

ForwardRange<const Info> Manager::getPlugins() const {
    return pimpl->getPlugins();
}

Signals::Plugin::PluginsLoaded& Manager::getPluginsLoadedSignal() const {
    return pimpl->pluginsLoaded;
}

} // namespace Plugin
} // namespace Melosic
