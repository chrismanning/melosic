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

module melosic.managers.common;

public import melosic.managers.input.inputmanager;
public import melosic.managers.output.outputmanager;

import std.string
,std.stdio
;
import core.sys.posix.dlfcn
;

class PluginException : Exception {
    this(string plugin, string msg, string file = __FILE__, size_t line = __LINE__) {
        super(plugin ~ ": " ~ msg, file, line);
    }
}

extern(C++) interface IKernel {
    IInputManager getDecoderManager();
}

class Kernel : IKernel {
    this() {
        inman = new InputManager;
    }

    void loadPlugin(string filename) {
        if(filename in loadedPlugins) {
            throw new PluginException(filename, "already loaded");
        }
        auto p = new Plugin(filename);
        p.registerPlugin(this);
        loadedPlugins[filename] = p;
    }

    extern(C++) IInputManager getDecoderManager() {
        return inman;
    }
private:
    Plugin[string] loadedPlugins;
    InputManager inman;
}

class Plugin {
public:
    this(string filename) {
        handle = .dlopen(toStringz(filename), RTLD_NOW);
        if(handle is null) {
            throw new PluginException(filename, "cannot open plugin");
        }
        registerPlugin_ = getSymbol!registerPlugin_T("registerPlugin");
        if(registerPlugin_ is null) {
            .dlclose(handle);
            throw new PluginException(filename, "cannot find symbol: registerPlugin");
        }
    }

    void registerPlugin(IKernel k) {
        registerPlugin_(k);
    }

    T getSymbol(T)(string sym) {
        return cast(T) dlsym(handle, toStringz(sym));
    }

private:
    alias extern(C) void function(IKernel) registerPlugin_T;
    registerPlugin_T registerPlugin_;
    void * handle;
}
