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

#include <iostream>
using std::cerr; using std::endl;
#include <map>
#include <boost/filesystem.hpp>
using boost::filesystem::path;

#include <melosic/core/inputmanager.hpp>
#include <melosic/managers/input/pluginterface.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/file.hpp>

namespace Melosic {
namespace Input {

class InputManager::impl {
public:
    Factory::result_type openFile(IO::File& file) {
        auto ext = path(file.filename()).extension().string();

        auto fact = factories.find(ext);

        enforceEx<Exception>(fact != factories.end(),
                                    [&file]() {
                                        return (file.filename() + ": cannot decode file").c_str();
                                    });
        return fact->second(file);
    }

    void addFactory(Factory fact, std::initializer_list<std::string> extensions) {
        BOOST_ASSERT(extensions.size() > 0);

        for(auto ext : extensions) {
            auto pos = factories.find(ext);
            if(pos == factories.end()) {
                factories.insert(decltype(factories)::value_type(ext, fact));
            }
            else {
                cerr << ext << ": can already be opened" << endl;
            }
        }
    }

private:
    std::map<std::string, Factory> factories;
};

InputManager::InputManager() : pimpl(new impl) {}

InputManager::~InputManager() {}

Factory::result_type InputManager::openFile(IO::File& file) {
    return pimpl->openFile(file);
}

void InputManager::addFactory(Factory fact, std::initializer_list<std::string> extensions)
{
    pimpl->addFactory(fact, extensions);
}

}
}
