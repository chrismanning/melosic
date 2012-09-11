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
#include <boost/iostreams/concepts.hpp>

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

enum class DeviceState {
    Error,
    Ready,
    Playing,
    Paused,
    Stopped,
};

class IDeviceSink : public ISink {
public:
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;
    virtual ~IDeviceSink() {}
    virtual void prepareDevice(Melosic::AudioSpecs& as) = 0;
    virtual const std::string& getDeviceDescription() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual DeviceState state() = 0;
    virtual std::streamsize write(const char* s, std::streamsize n) = 0;
};

class IFileSink : public ISink {
public:
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;
    virtual ~IFileSink() {}
    virtual std::streamsize write(const char* s, std::streamsize n) = 0;
};

}
}

#endif // MELOSIC_OUTPUT_PLUGINTERFACE_HPP
