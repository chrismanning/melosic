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

#ifndef MELOSIC_PLUGIN_HPP
#define MELOSIC_PLUGIN_HPP

#include <unistd.h>
#ifdef _POSIX_VERSION
#include <dlfcn.h>
#define DLHandle void*
#define DLOpen(a) dlopen(a, RTLD_NOW)
#define DLClose dlclose
#define DLGetSym dlsym
#endif

#ifdef _WIN32
#include <windows.h>
#define DLHandle HMODULE
#define DLOpen(a) LoadLibrary(a)
#define DLClose FreeLibrary
#define DLGetSym GetProcAddress
#endif

#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
using boost::filesystem::current_path;

#include <melosic/common/exports.hpp>
#include <melosic/common/error.hpp>

struct PluginException : public MelosicException {
    PluginException() : MelosicException(dlerror()) {
    }
};

struct PluginSymbolException : public PluginException {
    PluginSymbolException(DLHandle handle) {
        DLClose(handle);
    }
};

namespace Melosic {

class Plugin {
public:
    Plugin(const std::string& filename) {
        handle = DLOpen((current_path().string() + "/" + filename).c_str());

        enforceEx<PluginException>(handle != 0);

        registerPlugin_ = getFunction<registerPlugin_F>("registerPluginObjects");
        destroyPlugin_ = getFunction<destroyPlugin_F>("destroyPluginObjects");
    }

    ~Plugin() {
        if(destroyPlugin_) {
            destroyPlugin_();
        }
        if(handle) {
            DLClose(handle);
        }
    }

    void registerPluginObjects(IKernel& k) {
        registerPlugin_(k);
    }

private:
    template <typename T>
    std::function<T> getFunction(std::string symbolName) {
        auto sym = DLGetSym(handle, symbolName.c_str());
        enforceEx<PluginSymbolException>(sym != 0, handle);
        return reinterpret_cast<T*>(sym);
    }

    registerPlugin_T registerPlugin_;
    destroyPlugin_T destroyPlugin_;
    DLHandle handle;
};

} // end namespace Melosic

#endif // MELOSIC_PLUGIN_HPP
