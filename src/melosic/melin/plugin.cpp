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

#include <unistd.h>
#ifdef _POSIX_VERSION
#include <dlfcn.h>
#define DLHandle void*
#define DLOpen(a) dlopen(a, RTLD_NOW)
#define DLClose dlclose
#define DLGetSym dlsym
#define DLError dlerror
#endif

#ifdef WIN32
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
#endif

#include <map>
#include <memory>
#include <string>
#include <list>
#include <functional>
#include <thread>
using std::mutex; using std::unique_lock; using std::lock_guard;
namespace this_thread = std::this_thread;
#include <future>
#include <chrono>
namespace chrono = std::chrono;

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/core/kernel.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/error.hpp>
#include <melosic/melin/config.hpp>

#include "plugin.hpp"

namespace Melosic {
namespace Plugin {

class Plugin {
public:
    Plugin(const boost::filesystem::path& filename, Kernel& kernel)
        : pluginPath(absolute(filename)),
          logject(logging::keywords::channel = "Plugin"),
          kernel(kernel)
    {
        handle = DLOpen(pluginPath.c_str());

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
        LOG(logject) << "Plugin loaded: " << info;
    }

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    Plugin(Plugin&& b)
        : pluginPath(b.pluginPath),
          destroyPlugin_(b.destroyPlugin_),
          regFuns(b.regFuns),
          handle(b.handle),
          logject(b.logject),
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
        init_ = true;
    }

    bool initialised() const {
        return init_;
    }

private:
    template <typename T>
    std::function<T> getFunction(const std::string& symbolName) {
#ifdef _POSIX_VERSION
        DLError(); //clear err
#endif
        auto sym = DLGetSym(handle, symbolName.c_str());
        auto err = DLError();
#ifdef _POSIX_VERSION
        if(err != nullptr) {
#endif
#ifdef _WIN32
        if(sym == nullptr) {
#endif
            BOOST_THROW_EXCEPTION(PluginSymbolNotFoundException() <<
                                  ErrorTag::FilePath(pluginPath) <<
                                  ErrorTag::Plugin::Symbol(symbolName)<<
                                  ErrorTag::Plugin::Msg(err));
        }
        return reinterpret_cast<T*>(sym);
    }

    bool init_ = false;
    boost::filesystem::path pluginPath;
    destroyPlugin_T destroyPlugin_;
    std::list<std::function<void()>> regFuns;
    DLHandle handle;
    Logger::Logger logject;
    Kernel& kernel;
    Info info;
};

class Manager::impl {
public:
    impl(Kernel& kernel)
        : kernel(kernel),
          logject(logging::keywords::channel = "Plugin::Manager")
    {}

    void loadPlugin(const boost::filesystem::path& filepath) {
        if(!exists(filepath) && !is_regular_file(filepath)) {
            BOOST_THROW_EXCEPTION(FileNotFoundException() << ErrorTag::FilePath(absolute(filepath)));
        }

        if(filepath.extension() != ".melin") {
            BOOST_THROW_EXCEPTION(PluginInvalidException() << ErrorTag::FilePath(absolute(filepath)));
        }

        auto filename = filepath.filename().string();

        unique_lock<Mutex> l(mu);
        if(loadedPlugins.find(filename) != loadedPlugins.end()) {
            ERROR_LOG(logject) << "Plugin already loaded: " << filepath;
            return;
        }

        loadedPlugins.emplace(filename, Plugin(filepath, kernel));
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
        lock_guard<Mutex> l(mu);
        initialised_ = true;
    }

    bool initialised() {
        lock_guard<Mutex> l(mu);
        return initialised_;
    }

private:
    Kernel& kernel;
    std::map<std::string, Plugin> loadedPlugins;
    Logger::Logger logject;
    typedef mutex Mutex;
    Mutex mu;
    bool initialised_ = false;
    friend class Manager;
    friend struct RegisterFuncsInserter;
};

Manager::Manager(Kernel& kernel) : pimpl(new impl(kernel)) {}

Manager::~Manager() {}

void Manager::loadPlugin(const boost::filesystem::path& filepath) {
    pimpl->loadPlugin(filepath);
}

ForwardRange<const Info> Manager::getPlugins() const {
    return pimpl->getPlugins();
}

void Manager::initialise() {
    pimpl->initialise();
}

bool Manager::initialised() const {
    return pimpl->initialised();
}

} // namespace Plugin

RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerInput_T& fun) {
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerDecoder_T& fun) {
    e->push(std::bind(fun, &k.getDecoderManager()));
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerOutput_T& fun) {
    e->push(std::bind(fun, &k.getOutputManager()));
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerEncoder_T& fun) {
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerSlots_T& fun) {
    e->push(std::bind(fun, &k.getSlotManager()));
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerConfig_T& fun) {
    e->push(std::bind(fun, &k.getConfigManager()));
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerTasks_T& fun) {
    e->push(std::bind(fun, &k.getThreadManager()));
    return *this;
}

} // namespace Melosic
