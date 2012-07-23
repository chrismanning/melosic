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

module melosic.managers.input.inputmanager;

public import melosic.managers.input.pluginterface
;
import std.string
,std.stdio
;
import core.memory
;

extern(C++) interface IAudioFile {
}

extern(C++) interface IInputManager {
    void addInputSource(IInputSource dec);
    InputSource openFile(string filename);
}

class InputManager : IInputManager {
public:
    //FIXME: there must be a better scheme than returning an InputSource here
    extern(C++) InputSource openFile(string filename) {
        foreach(src; srcs) {
            if(src.canOpen(filename)) {
                debug writeln(filename, " can be opened");
                src.openFile(filename);
                return src;
            }
        }
        throw new Exception("cannot open file " ~ filename);
    }

    extern(C++) void addInputSource(IInputSource src) {
        srcs ~= new InputSource(src);
    }

private:
    InputSource[] srcs;
}
