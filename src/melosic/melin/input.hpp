/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#ifndef MELOSIC_INPUTMANAGER_HPP
#define MELOSIC_INPUTMANAGER_HPP

#include <memory>

#include <melosic/common/common.hpp>
#include <melosic/common/stream.hpp>

namespace Melosic {
struct AudioSpecs;
namespace Input {

class Manager {
public:
    Manager();
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class Playable : public IO::Source {
public:
    typedef char char_type;
    virtual ~Playable() {}
    virtual void seek(chrono::milliseconds dur) = 0;
    virtual chrono::milliseconds tell() = 0;
    virtual chrono::milliseconds duration() const = 0;
    virtual Melosic::AudioSpecs getAudioSpecs() = 0;
    virtual explicit operator bool() = 0;
    virtual void reset() = 0;
};

} // namespace Input
} // namespace Melosic

#endif // MELOSIC_INPUTMANAGER_HPP
