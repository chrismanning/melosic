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

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <initializer_list>
#include <string>
#include <boost/functional/factory.hpp>

#include <melosic/managers/input/pluginterface.hpp>

class IInputSource;

class IInputManager {
public:
    virtual void addFactory(std::function<std::shared_ptr<IInputSource>()> fact,
                            std::initializer_list<const std::string> extensions);
    virtual std::shared_ptr<IInputSource> openFile(const std::string& filename);
};

#endif // INPUT_MANAGER_H
