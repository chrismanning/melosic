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

#include <list>
#include <boost/filesystem.hpp>

#include <melosic/managers/input/pluginterface.hpp>

using boost::filesystem::path;

class InputManager : IInputManager {
public:
    virtual std::shared_ptr<IInputSource> openFile(std::string const& filename) {
        for(auto factory : factories) {
            if(factory->canOpen(path(filename).extension().string())) {
                cerr << filename << " can be opened" << endl;
                auto tmp = factory->create();
                tmp->openFile(filename);
                return tmp;
            }
        }
        cerr << "Cannot open file: " << filename << endl;
        return 0;
        //throw new Exception("cannot open file " ~ filename);
    }

    virtual void addInputFactory(IInputFactory * factory) {
        factories.push_back(factory);
    }

private:
    std::list<IInputFactory*> factories;
};
