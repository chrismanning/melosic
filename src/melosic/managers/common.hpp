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

#include <iostream>
using std::cout; using std::cerr; using std::endl;

#include <melosic/managers/input/inputmanager.hpp>
#include <melosic/managers/output/outputmanager.hpp>

typedef unsigned char ubyte;
typedef signed char byte;
typedef unsigned uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

class ErrorHandler {
public:
    virtual void report(const char * msg, const char * file = __FILE__, uint line = __LINE__) = 0;
};

class IInputManager;

class IKernel {
public:
    virtual IInputManager * getInputManager() = 0;
    virtual IOutputManager * getOutputManager() = 0;
    virtual void loadAllPlugins() = 0;
};

extern "C" void registerPluginObjects(IKernel * k);
extern "C" void destroyPluginObjects();
extern "C" int startEventLoop(int argc, char ** argv, IKernel * k);

struct IBuffer {
    virtual const void * ptr() = 0;
    virtual size_t length() = 0;
    virtual void ptr(const void * p) = 0;
    virtual void length(size_t p) = 0;
};

class Buffer : public IBuffer {
public:
    Buffer() : ptr_(0), length_(0) {}

    virtual const void * ptr() {
        return ptr_;
    }

    virtual size_t length() {
        return length_;
    }

    virtual void ptr(const void * p) {
        ptr_ = p;
    }

    virtual void length(size_t p) {
        length_ = p;
    }

private:
    const void * ptr_;
    size_t length_;
};

struct AudioSpecs {
    AudioSpecs() : channels(0), bps(0) , sample_rate(0), total_samples(0) {}
    AudioSpecs(ubyte channels, ubyte bps, uint sample_rate, ulong total_samples)
        : channels(channels), bps(bps), sample_rate(sample_rate), total_samples(total_samples) {}
    ubyte channels;
    ubyte bps;
    uint sample_rate;
    ulong total_samples;
};

class FileHandler {

};

enum PlaybackState {
    Playing,
    Paused,
    Stopped,
};

class PlaybackHandler : public ErrorHandler {
public:
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual PlaybackState state() = 0;
    virtual IBuffer * requestData(uint bytes) = 0;
};

#endif // COMMON_H
