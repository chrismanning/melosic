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

#ifndef MELOSIC_INPUT_PLUGINTERFACE_H
#define MELOSIC_INPUT_PLUGINTERFACE_H

#include <chrono>
#include <boost/iostreams/concepts.hpp>
#include <melosic/common/stream.hpp>

namespace Melosic {

struct AudioSpecs;
struct IBuffer;

namespace Input {

class ISource : public IO::SeekableSource {
public:
    typedef char char_type;
    virtual ~ISource() {}
    virtual void seek(std::chrono::milliseconds dur) = 0;
    virtual std::chrono::milliseconds tell() = 0;
    virtual Melosic::AudioSpecs& getAudioSpecs() = 0;
    virtual explicit operator bool() = 0;
};

class IFileSource : public ISource {
public:
    virtual ~IFileSource() {}
};

}
}

#endif // MELOSIC_INPUT_PLUGINTERFACE_H
