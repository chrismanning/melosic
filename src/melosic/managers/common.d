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

import
std.string
,std.stdio
,std.conv
,std.bitmanip
,std.file
,std.algorithm
;
public import
core.sys.posix.dlfcn
;

alias nativeToLittleEndian ntl;
alias nativeToBigEndian ntb;

class PluginException : Exception {
    this(string plugin, string msg, string file = __FILE__, size_t line = __LINE__) {
        super(plugin ~ ": " ~ msg, file, line);
    }
}

extern(C++) interface IKernel {
    IInputManager getInputManager();
    IOutputManager getOutputManager();
    void loadAllPlugins();
}

class Kernel : IKernel {
    this() {
        inman = new InputManager;
        outman = new OutputManager;
    }

    void loadPlugin(string filename) {
        if(filename in loadedPlugins) {
            //FIXME: use future error handling capabilities
            throw new PluginException(filename, "already loaded");
        }
        auto p = new Plugin(filename);
        p.registerPluginObjects(this);
        loadedPlugins[filename] = p;
    }

    extern(C++) void loadAllPlugins() {
        foreach(pe; dirEntries("plugins", "*.so", SpanMode.depth)) {
            if(pe.name.canFind("qt")) {
                continue;
            }
            loadPlugin(pe.name);
        }
    }

    extern(C++) IInputManager getInputManager() {
        return inman;
    }

    extern(C++) IOutputManager getOutputManager() {
        return outman;
    }

private:
    Plugin[string] loadedPlugins;
    InputManager inman;
    OutputManager outman;
}

extern(C) void registerPluginObjects(IKernel k);
extern(C) void destroyPluginObjects();
extern(C) int startEventLoop(int argc, char ** argv, IKernel k);

class Plugin {
public:
    this(string filename) {
        handle = .dlopen(filename.toStringz(), RTLD_NOW);
        if(handle is null) {
            //FIXME: use future error handling capabilities
            throw new Exception(to!string(dlerror()));
        }
        registerPlugin_ = getSymbol!registerPlugin_T("registerPluginObjects");
        if(registerPlugin_ is null) {
            .dlclose(handle);
            //FIXME: use future error handling capabilities
            throw new PluginException(filename, "cannot find symbol: registerPluginObjects");
        }
        destroyPlugin_ = getSymbol!destroyPlugin_T("destroyPluginObjects");
        if(destroyPlugin_ is null) {
            .dlclose(handle);
            //FIXME: use future error handling capabilities
            throw new PluginException(filename, "cannot find symbol: destroyPluginObjects");
        }
    }

    ~this() {
        if(destroyPlugin_ !is null) {
            destroyPlugin_();
        }
        if(handle !is null) {
            .dlclose(handle);
        }
    }

    void registerPluginObjects(IKernel k) {
        registerPlugin_(k);
    }

    T getSymbol(T)(string sym) {
        return cast(T) .dlsym(handle, toStringz(sym));
    }

private:
    alias extern(C) void function(IKernel) registerPlugin_T;
    registerPlugin_T registerPlugin_;
    alias extern(C) void function() destroyPlugin_T;
    destroyPlugin_T destroyPlugin_;
    void * handle;
}

extern(C++) interface IBuffer {
    const(void *) ptr();
    size_t length();
    final const(ubyte[]) opSlice() {
        return cast(const(ubyte[]))ptr()[0..length()];
    }
}

extern(C) struct AudioSpecs {
    this(ubyte channels, ubyte bps, uint sample_rate, ulong total_samples) {
        this.channels = channels;
        this.bps = bps;
        this.sample_rate = sample_rate;
        this.total_samples = total_samples;
    }
    ubyte channels;
    ubyte bps;
    uint sample_rate;
    ulong total_samples;
}

//TODO: define error handling
extern(C++) interface ErrorHandler {
}
