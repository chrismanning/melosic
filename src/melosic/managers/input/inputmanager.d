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

import melosic.managers.input.pluginterface.d
;
import std.string
,std.stdio
;
import core.memory
;

extern(C++) interface IAudioFile {
}

extern(C++) interface IInputManager {
    void addDecoder(IInputDecoder dec);
    InputDecoder openFile(string filename);
}

class InputManager : IInputManager {
  public:
    //FIXME: there must be a better scheme than returning an InputDecoder here
    extern(C++) InputDecoder openFile(string filename) {
        foreach(dec; decoders) {
            if(dec.canOpen(filename)) {
                debug writeln(filename, " can be opened");
                dec.openFile(filename);
                return dec;
            }
        }
        assert(0);
    }

    extern(C++) void addDecoder(IInputDecoder dec) {
        GC.addRoot(cast(const(void*))dec);
        decoders ~= new InputDecoder(dec);
    }

    InputDecoder[] decoders;
}
