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

#ifndef INPUT_PLUGINTERFACE_H
#define INPUT_PLUGINTERFACE_H

#include <cstddef>
#include <melosic/managers/common.hpp>

struct AudioSpecs;
struct IBuffer;

class DecodeRange {
public:
    virtual IBuffer * front() = 0;
    virtual void popFront() = 0;
    virtual bool empty() = 0;
    virtual size_t length() = 0;
};

class IInputSource {
public:
    virtual void openFile(const char * filename) = 0;
    virtual DecodeRange * getDecodeRange() = 0;
    virtual AudioSpecs getAudioSpecs() = 0;
    virtual void writeBuf(const void * ptr, size_t length) = 0;
    virtual void destroyRange(DecodeRange * range) = 0;
};

class IInputFactory {
public:
    virtual ~IInputFactory() {}
    virtual bool canOpen(const char * extension) = 0;
    virtual IInputSource * create() = 0;
    virtual void destroy(IInputSource * ptr) = 0;
};

#endif // INPUT_PLUGINTERFACE_H
