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

#include <memory>
#include <vector>
#include <ios>
#include <boost/iostreams/concepts.hpp>

namespace Melosic {

struct AudioSpecs;
struct IBuffer;

namespace Input {

class ISource {
public:
    virtual ~ISource() {}
    virtual std::streamsize read(uint8_t * s, std::streamsize n) = 0;
    virtual const Melosic::AudioSpecs& getAudioSpecs() = 0;
};

class IFileSource : public ISource, boost::iostreams::source {
public:
    virtual ~IFileSource() {}
    virtual void openFile(const std::string& filename) = 0;
};

}
}

#endif // MELOSIC_INPUT_PLUGINTERFACE_H
