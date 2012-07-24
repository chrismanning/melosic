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

import
std.stdio
;
import
melosic.managers.common
,melosic.core.wav
;

void main(string args[]) {
    writeln("Hello World!");
    auto kernel = new Kernel;
    try {
        kernel.loadPlugin("plugins/flac/flac.so");
        kernel.loadPlugin("plugins/alsa/alsa.so");
        auto decoder = kernel.getInputManager().openFile("test.flac");
//        auto output = kernel.getOutputManager().getDefaultOutput();
        auto output = new WaveFile("test1.wav");
        output.prepareDevice(decoder.getAudioSpecs());
        output.render(decoder[]);
    }
    catch(Exception e) {
        stderr.writeln(e.msg);
    }
}
