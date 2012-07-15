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

module melosic.main;

import std.stdio
;
import melosic.managers.common
;

void main() {
    writeln("Hello World!");
    auto kernel = new Kernel;
    try {
        kernel.loadPlugin("plugins/flac/flac.so");
        kernel.getDecoderManager().openFile("blah.flac");
    }
    catch(PluginException e) {
        stderr.writeln(e.msg);
    }
}