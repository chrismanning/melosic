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

module melosic.managers.output.pluginterface;

import
std.conv
,std.stdio
;

public import
melosic.managers.common
;

extern(C++) interface IOutput {
    void prepareDevice(AudioSpecs as);
    const(char *) getDeviceDescription();
    const(char *) getDeviceName();
    DeviceCapabilities * getDeviceCapabilities();
    void render(DecodeRange src);
}

class OutputDevice {
    this(IOutput iod) {
        this.iod = iod;
    }

    void prepareDevice(AudioSpecs as) {
        iod.prepareDevice(as);
    }

    string getDeviceDescription() {
        return to!string(iod.getDeviceDescription());
    }

    DeviceCapabilities * getDeviceCapabilities() {
        return iod.getDeviceCapabilities();
    }

    void render(DecodeRange src) {
        iod.render(src);
    }

    string getDeviceName() {
        return to!string(iod.getDeviceName());
    }

    IOutput iod;
}

extern(C) struct DeviceCapabilities {
}
