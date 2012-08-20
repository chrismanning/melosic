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
#define DLError dlerror
#endif

#ifdef _WIN32
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

#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
using boost::filesystem::absolute;

#include <melosic/common/exports.hpp>
#include <melosic/common/error.hpp>

namespace Melosic {

struct PluginException : public Exception {
    PluginException() : Exception(DLError()) {
    }
};

struct PluginSymbolException : public PluginException {
    PluginSymbolException(DLHandle handle) {
        DLClose(handle);
    }
};

class Plugin {
public:
    Plugin(const boost::filesystem::path& filename) {
        handle = DLOpen(absolute(filename).string().c_str());

        enforceEx<PluginException>(handle != 0);

        registerPlugin_ = getFunction<registerPlugin_F>("registerPluginObjects");
        destroyPlugin_ = getFunction<destroyPlugin_F>("destroyPluginObjects");
    }

    ~Plugin() {
        if(handle) {
            if(destroyPlugin_) {
                destroyPlugin_();
            }
            DLClose(handle);
        }
    }

    void registerPluginObjects(Kernel& k) {
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
