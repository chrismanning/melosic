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
#include <boost/exception/error_info.hpp>

#include <melosic/common/stream.hpp>

namespace Melosic {

struct AudioSpecs;

namespace ErrorTag {
namespace Output {
typedef boost::error_info<struct tagDeviceErrStr, std::string> DeviceErrStr;
}
}

namespace Output {

struct DeviceCapabilities {
};

class Sink : public IO::Sink {
public:
    virtual ~Sink() {}
    virtual const std::string& getSinkName() = 0;
    virtual void prepareSink(Melosic::AudioSpecs& as) = 0;
};

enum class DeviceState {
    Error,
    Ready,
    Playing,
    Paused,
    Stopped,
};

class DeviceSink : public Sink {
public:
    virtual ~DeviceSink() {}
    virtual const Melosic::AudioSpecs& currentSpecs() = 0;
    virtual const std::string& getSinkDescription() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual DeviceState state() = 0;
};

class FileSink : public Sink, IO::Closable {
public:
    virtual ~FileSink() {}
};

}
}

#endif // MELOSIC_OUTPUT_PLUGINTERFACE_HPP
