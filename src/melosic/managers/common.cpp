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
#include <functional>

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

#include <melosic/managers/input/inputmanager.hpp>
#include <melosic/managers/output/outputmanager.hpp>

extern "C" void registerPluginObjects(IKernel * k);
extern "C" void destroyPluginObjects();
typedef void (*registerPlugin_T)(IKernel*);
typedef void (*destroyPlugin_T)();

class Plugin {
public:
    Plugin(std::string filename) {
        handle = DLOpen(filename.c_str());
        if(!handle) {
            //FIXME: use future error handling capabilities
//            std::cerr << dlerror() << std::endl;
        }
        registerPlugin_ = getSymbol<registerPlugin_T>("registerPluginObjects");
        if(!registerPlugin_) {
            //FIXME: use future error handling capabilities
//            std::cerr << dlerror() << std::endl;
            DLClose(handle);
//            throw new Exception(str);
        }
        destroyPlugin_ = getSymbol<destroyPlugin_T>("destroyPluginObjects");
        if(!destroyPlugin_) {
            //FIXME: use future error handling capabilities
//            std::cerr << dlerror() << std::endl;
            DLClose(handle);
//            throw new Exception(str);
        }
    }

    ~Plugin() {
        if(destroyPlugin_) {
            destroyPlugin_();
        }
        if(handle) {
            DLClose(handle);
        }
    }

    void registerPluginObjects(IKernel * k) {
        registerPlugin_(k);
    }

    template <typename T>
    T getSymbol(std::string sym) {
        return reinterpret_cast<T>(DLGetSym(handle, sym.c_str()));
    }

private:
    registerPlugin_T registerPlugin_;
    destroyPlugin_T destroyPlugin_;
    DLHandle handle;
};

class Kernel : public IKernel {
    Kernel() {
//        inman = new InputManager;
//        outman = new OutputManager;
    }

    void loadPlugin(std::string filename) {
//        if(filename in loadedPlugins) {
//            stderr.writefln("Plugin already loaded: %s", filename);
//        }
        auto p = new Plugin(filename);
        p->registerPluginObjects(this);
        std::pair<std::string, Plugin*> tmp(filename, p);
        loadedPlugins.insert(tmp);
    }

   void loadAllPlugins() {
//        foreach(pe; dirEntries("plugins", "*.so", SpanMode.depth)) {
//            if(pe.name.canFind("qt")) {
//                continue;
//            }
//            loadPlugin(pe.name);
//        }
    }

    IInputManager * getInputManager() {
        return inman;
    }

    IOutputManager * getOutputManager() {
        return outman;
    }

private:
    std::map<std::string, Plugin*> loadedPlugins;
    IInputManager * inman;
    IOutputManager * outman;
};
