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

module melosic.main;

import
std.stdio
,std.conv
,std.string
;
import
melosic.managers.common
,melosic.core.wav
;

alias extern(C) int function(int argc, char ** argv, IKernel k) startEventLoop_T;

int main(string args[]) {
    auto kernel = new shared(Kernel);

    auto handle = .dlopen(toStringz("plugins/qt-gui/qt-gui.so"), RTLD_NOW);
    if(handle is null) {
        stderr.writeln(to!string(dlerror()));
        return -1;
    }
    stderr.writeln("Loaded Qt GUI plugin");

    auto startEventLoop = cast(startEventLoop_T).dlsym(handle, toStringz("startEventLoop"));
    if(startEventLoop is null) {
        stderr.writeln(to!string(dlerror()));
        .dlclose(handle);
        return -1;
    }

    char*[] tmp;
    foreach(arg; args) {
        tmp ~= cast(char*)toStringz(arg);
    }

    return startEventLoop(cast(int)args.length, tmp.ptr, kernel);
}
