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

namespace Melosic {

struct AudioSpecs;
struct IBuffer;

}

class IInputSource {
public:
    virtual ~IInputSource() {}
    virtual void openFile(const std::string& filename) = 0;
    virtual const Melosic::AudioSpecs& getAudioSpecs() = 0;
    virtual void writeBuf(const std::vector<uint8_t>& buf, size_t length) = 0;
};

class IInputFactory {
public:
    virtual ~IInputFactory() {}
    virtual bool canOpen(const std::string& extension) = 0;
    virtual std::shared_ptr<IInputSource> create() = 0;
};

#endif // MELOSIC_INPUT_PLUGINTERFACE_H
