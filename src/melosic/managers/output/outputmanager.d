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

module melosic.managers.output.outputmanager;

import
std.algorithm
,std.exception
;

public import melosic.managers.output.pluginterface;

extern(C++) interface IOutputManager {
    void addOutput(IOutput dev);
    OutputDevice getDefaultOutput();
}

class OutputManager : IOutputManager {
    extern(C++) void addOutput(IOutput dev) {
        devs ~= new OutputDevice(dev);
    }

    extern(C++) OutputDevice getDefaultOutput() {
        auto r = filter!(a => a.getDeviceName().canFind("default"))(devs);
        enforceEx!Exception(r.count() > 0, "Cannot find default device");
        return r.front();
    }

private:
    OutputDevice[] devs;
}
