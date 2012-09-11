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

#ifndef MELOSIC_INPUT_MANAGER_H
#define MELOSIC_INPUT_MANAGER_H

#include <memory>

namespace Melosic {

namespace IO {
class File;
class SeekableSource;
}

namespace Input {

class ISource;

typedef std::function<std::shared_ptr<ISource>(IO::SeekableSource&)> Factory;

class InputManager {
public:
    InputManager();
    ~InputManager();
    Factory::result_type openFile(IO::File& file);
    Factory getFactory(const IO::File& file);
    void addFactory(Factory fact, std::initializer_list<std::string> extensions);
private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
}

#endif // MELOSIC_INPUT_MANAGER_H
