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

#ifndef MELOSIC_IINPUT_MANAGER_H
#define MELOSIC_IINPUT_MANAGER_H

#include <initializer_list>
#include <string>
#include <memory>

#include <melosic/managers/input/pluginterface.hpp>

namespace Melosic {
namespace Input {

class IInputManager {
public:
    virtual ISource&& openFile(const std::string& filename) = 0;
    virtual void addFactory(std::function<ISource&&()>&& fact,
                            std::initializer_list<std::string> extensions) = 0;
};

}
}

#endif // MELOSIC_IINPUT_MANAGER_H
