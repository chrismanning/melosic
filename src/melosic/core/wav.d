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

import
std.stdio
,std.bitmanip
,std.string
;
import
core.memory
;
import
melosic.managers.common
;

class WaveFile : IOutput {
    this(string filename) {
        file = File(filename, "w");
    }

    extern(C++) void prepareDevice(AudioSpecs as) {
        this.as = as;
    }

    extern(C++) const(char *) getDeviceDescription() {
        return toStringz("Wav file");
    }

    extern(C++) const(char *) getDeviceName() {
        return toStringz(file.name);
    }

    extern(C++) DeviceCapabilities * getDeviceCapabilities() {
        return new DeviceCapabilities;
    }

    extern(C++) void render(DecodeRange src) {
        uint total_size = cast(uint)as.total_samples * as.channels * (as.bps/8);
        file.rawWrite("RIFF");
        file.rawWrite(nativeToLittleEndian!uint(total_size + 36));
        file.rawWrite("WAVEfmt ");
        file.rawWrite(nativeToLittleEndian!uint(16));
        file.rawWrite(nativeToLittleEndian!ushort(1));
        file.rawWrite(nativeToLittleEndian!ushort(as.channels));
        file.rawWrite(nativeToLittleEndian!uint(as.sample_rate));
        ushort q = as.channels * (as.bps/8);
        file.rawWrite(nativeToLittleEndian!uint(as.sample_rate * q));
        file.rawWrite(nativeToLittleEndian!ushort(q));
        file.rawWrite(nativeToLittleEndian!ushort(as.bps));
        file.rawWrite("data");
        file.rawWrite(nativeToLittleEndian!uint(total_size));
        file.flush();
        stderr.writeln("Written header");

        FILE * fp = file.getFP();

        src.popFront();
        while(!src.empty()) {
            auto f = src.front();
            fwrite(src.front().ptr(), 1, src.front().length(), fp);
            src.popFront();
        }
    }

    File file;
    ubyte sample_number;
    AudioSpecs as;
    ubyte[] header;
}

//class RawPCMFileOutputRange : IOutputRange {
//    this(string filename) {
//        file = File(filename, "w+");
//    }

//    extern(C++) bool put(int a, ubyte bps) {
//        ubyte[] output;
//        switch(bps) {
//            case 8:
//                output = nativeToLittleEndian(cast(byte)a)[];
//                break;
//            case 16:
//                output = nativeToLittleEndian(cast(short)a)[];
//                break;
//            case 24:
//                output = nativeToLittleEndian!byte(cast(byte)a)[];
//                break;
//            case 32:
//                output = nativeToLittleEndian!int(a)[];
//                break;
//            default:
//                return false;
//        }
//        try {
//            file.rawWrite(output);
//            return true;
//        }
//        catch(Exception e) {
//            return false;
//        }
//    }

//    File file;
//}
