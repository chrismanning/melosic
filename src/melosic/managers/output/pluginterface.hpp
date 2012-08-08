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

#ifndef MELOSIC_OUTPUT_PLUGINTERFACE_HPP
#define MELOSIC_OUTPUT_PLUGINTERFACE_HPP

#include <string>

namespace Melosic {

class PlaybackHandler;
struct AudioSpecs;

namespace Output {

struct DeviceCapabilities {
};

class ISink {
public:
    virtual ~ISink() {}
    virtual const std::string& getSinkName() = 0;
};

class IDeviceSink : public ISink {
public:
    virtual ~IDeviceSink() {}
    virtual void prepareDevice(Melosic::AudioSpecs as) = 0;
    virtual const std::string& getDeviceDescription() = 0;
    virtual void render(Melosic::PlaybackHandler * playHandle) = 0;
};

class IFileSink : public ISink {
public:
    virtual ~IFileSink() {}
    virtual void render() = 0;
};

}
}

#endif // MELOSIC_OUTPUT_PLUGINTERFACE_HPP
