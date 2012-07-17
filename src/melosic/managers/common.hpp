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

#ifndef COMMON_H
#define COMMON_H

#include <melosic/managers/input/inputmanager.hpp>
#include <cstddef>

typedef unsigned char ubyte;
typedef signed char byte;
typedef unsigned uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

class IKernel {
public:
    virtual IInputManager * getInputManager() = 0;
};

class IOutputRange {
public:
    virtual bool put(int a, ubyte bps);
};

extern "C" struct AudioSpecs {
    AudioSpecs(ubyte channels, ubyte bps, ulong total_samples, uint sample_rate)
        : channels(channels), bps(bps), total_samples(total_samples), sample_rate(sample_rate) {}
    ubyte channels;
    ubyte bps;
    ulong total_samples;
    uint sample_rate;
};

#endif // COMMON_H
