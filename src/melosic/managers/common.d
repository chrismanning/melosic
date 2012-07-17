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
;
import
core.sys.posix.dlfcn
;

class PluginException : Exception {
    this(string plugin, string msg, string file = __FILE__, size_t line = __LINE__) {
        super(plugin ~ ": " ~ msg, file, line);
    }
}

extern(C++) interface IKernel {
    IInputManager getInputManager();
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

    extern(C++) IInputManager getInputManager() {
        return inman;
    }

private:
    Plugin[string] loadedPlugins;
    InputManager inman;
}

extern(C) void registerPlugin(IKernel kernel);

class Plugin {
public:
    this(string filename) {
        handle = .dlopen(filename.toStringz(), RTLD_NOW);
        if(handle is null) {
            throw new Exception(to!string(dlerror()));
        }
        registerPlugin_ = getSymbol!registerPlugin_T("registerPlugin");
        if(registerPlugin_ is null) {
            .dlclose(handle);
            throw new PluginException(filename, "cannot find symbol: registerPlugin");
        }
    }

    ~this() {
        if(handle !is null) {
            .dlclose(handle);
        }
    }

    void registerPlugin(IKernel k) {
        registerPlugin_(k);
    }

    T getSymbol(T)(string sym) {
        return cast(T) .dlsym(handle, toStringz(sym));
    }

private:
    alias extern(C) void function(IKernel) registerPlugin_T;
    registerPlugin_T registerPlugin_;
    void * handle;
}

extern(C++) interface IOutputRange {
    bool put(int a, ubyte bps);
}

import std.bitmanip;

class RawPCMFileOutputRange : IOutputRange {
    this(string filename) {
        file = File(filename, "w+");
    }

    extern(C++) bool put(int a, ubyte bps) {
        ubyte[] output;
        switch(bps) {
            case 8:
                output = nativeToLittleEndian(cast(byte)a)[];
                break;
            case 16:
                output = nativeToLittleEndian(cast(short)a)[];
                break;
            case 24:
                output = nativeToLittleEndian!byte(cast(byte)a)[];
                break;
            case 32:
                output = nativeToLittleEndian!int(a)[];
                break;
            default:
                return false;
        }
        try {
            file.rawWrite(output);
            return true;
        }
        catch(Exception e) {
            return false;
        }
    }

    File file;
}

extern(C) struct AudioSpecs {
    this(ubyte channels, ubyte bps, ulong total_samples, uint sample_rate) {
        this.channels = channels;
        this.bps = bps;
        this.total_samples = total_samples;
        this.sample_rate = sample_rate;
    }
    ubyte channels;
    ubyte bps;
    ulong total_samples;
    uint sample_rate;
}

class WaveFileOutputRange : IOutputRange {
    this(string filename) {
        file = File(filename, "w+");
    }

    extern(C++) bool put(int a, ubyte bps) {
        if(sample_number == 0) {
            try {
                file.rawWrite("RIFF");
                //TODO: wave file output
//                !write_little_endian_uint32(f, total_size + 36) ||
//                fwrite("WAVEfmt ", 1, 8, f) < 8 ||
//                !write_little_endian_uint32(f, 16) ||
//                !write_little_endian_uint16(f, 1) ||
//                !write_little_endian_uint16(f, cast(ushort) channels) ||
//                !write_little_endian_uint32(f, sample_rate) ||
//                !write_little_endian_uint32(f, sample_rate * channels * (bps/8)) ||
//                !write_little_endian_uint16(f, cast(ushort)(channels * (bps/8))) || /* block align */
//                !write_little_endian_uint16(f, cast(ushort)bps) ||
//                fwrite("data", 1, 4, f) < 4 ||
//                !write_little_endian_uint32(f, total_size);
            }
            catch(Exception e) {
                return false;
            }
            sample_number++;
        }
        return true;
    }

    File file;
    ubyte sample_number;
}
