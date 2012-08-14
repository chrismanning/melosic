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

#include <iostream>
using std::cerr; using std::endl;
#include <map>
#include <boost/filesystem.hpp>

#include <melosic/managers/input/iinputmanager.hpp>
#include <melosic/managers/input/pluginterface.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/file.hpp>

using boost::filesystem::path;

namespace Melosic {
namespace Input {

class InputManager : public IInputManager {
public:
    virtual std::shared_ptr<IFileSource> openFile(IO::File& file) {
        auto ext = path(file.filename()).extension().string();

        auto fact = factories.find(ext);

        enforceEx<Exception>(fact != factories.end(),
                                    [&file]() {
                                        return (file.filename() + ": cannot decode file").c_str();
                                    });
        return fact->second(file);
    }

    virtual void addFactory(std::function<std::shared_ptr<IFileSource>(IO::BiDirectionalSeekable&)> fact,
                                 std::initializer_list<std::string> extensions) {
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
    std::map<std::string,std::function<std::shared_ptr<IFileSource>(IO::File&)> > factories;
};

}
}

#endif // MELOSIC_INPUT_MANAGER_H
