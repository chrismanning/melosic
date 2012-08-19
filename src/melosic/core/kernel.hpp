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

#ifndef MELOSIC_CORE_KERNEL_HPP
#define MELOSIC_CORE_KERNEL_HPP

#include <memory>

namespace Melosic {

namespace Input {
class InputManager;
}

namespace Output {
class OutputManager;
}

class Kernel {
public:
    Kernel();
    ~Kernel();
    void loadPlugin(const std::string& filepath);
    void loadAllPlugins();
    Input::InputManager& getInputManager();
    Output::OutputManager& getOutputManager();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}

#endif // MELOSIC_CORE_KERNEL_HPP
