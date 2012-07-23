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

#ifndef OUTPUT_PLUGINTERFACE_HPP
#define OUTPUT_PLUGINTERFACE_HPP

#include <melosic/managers/common.hpp>

extern "C" struct DeviceCapabilities {
};

class IOutput {
public:
    virtual void prepareDevice(AudioSpecs as) = 0;
    virtual const char * getDeviceDescription() = 0;
    virtual const char * getDeviceName() = 0;
    virtual const DeviceCapabilities * getDeviceCapabilities() = 0;
    virtual void render(DecodeRange * src) = 0;
};

#endif // OUTPUT_PLUGINTERFACE_HPP
