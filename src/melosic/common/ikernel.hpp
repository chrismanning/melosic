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

#ifndef MELOSIC_COMMON_IKERNEL_HPP
#define MELOSIC_COMMON_IKERNEL_HPP

#include <map>

#include <melosic/common/plugin.hpp>
#include <melosic/managers/input/iinputmanager.hpp>
#include <melosic/managers/output/ioutputmanager.hpp>

namespace Melosic {

class IKernel {
public:
    virtual void loadPlugin(const std::string& filename) = 0;
    virtual Input::IInputManager& getInputManager() = 0;
    virtual Output::IOutputManager& getOutputManager() = 0;
};

} // end namespace Melosic

#endif // MELOSIC_COMMON_IKERNEL_HPP
