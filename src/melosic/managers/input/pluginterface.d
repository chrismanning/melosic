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

module melosic.managers.input.pluginterface;

import
std.string
,std.path
;
import
core.memory
;

public import
melosic.managers.common
;

extern(C++) interface IInputSource {
public:
    bool canOpen(const(char *) extension);
    void openFile(const(char *) filename);
    DecodeRange getDecodeRange();
    AudioSpecs getAudioSpecs();
    void writeBuf(const(void *) ptr, size_t length);
}

class InputSource {
    this(IInputSource iid) {
        this.iid = iid;
    }

    bool canOpen(string filename) {
        return iid.canOpen(filename.extension().toStringz());
    }

    void openFile(string filename) {
        iid.openFile(filename.toStringz());
    }

    AudioSpecs getAudioSpecs() {
        return iid.getAudioSpecs();
    }

    DecodeRange opSlice() {
        auto x = iid.getDecodeRange();
        GC.addRoot(cast(const(void*))x);
        return x;
    }

    IInputSource iid;
}

extern(C++) interface DecodeRange {
    @property IBuffer front();
    void popFront();
    @property bool empty();
    size_t length();
}
