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

#include <boost/chrono.hpp>
namespace chrono = boost::chrono;
#include <boost/iostreams/concepts.hpp>
#include <melosic/common/stream.hpp>
#include <melosic/common/error.hpp>

namespace Melosic {

namespace ErrorTag {
typedef boost::error_info<struct tagDecoderStr, std::string> DecodeErrStr;
}

struct AudioSpecs;

namespace Input {

class Source : public IO::Source {
public:
    typedef char char_type;
    virtual ~Source() {}
    virtual void seek(chrono::milliseconds dur) = 0;
    virtual chrono::milliseconds tell() = 0;
    virtual Melosic::AudioSpecs& getAudioSpecs() = 0;
    virtual explicit operator bool() = 0;
    virtual void reset() = 0;
};

class FileSource : public Source {
    virtual const std::string& getFilename() = 0;
};

}
}

#endif // MELOSIC_INPUT_PLUGINTERFACE_H
