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

#ifndef MELOSIC_WAV_HPP
#define MELOSIC_WAV_HPP

#include <boost/filesystem.hpp>
using boost::filesystem::absolute;
#include <fstream>

#include <melosic/common/common.hpp>

namespace Melosic {

class WaveFile {
public:
    WaveFile(const boost::filesystem::path& filename, AudioSpecs as)
        : filepath(absolute(filename))
        , file(filepath.string(), std::ios_base::out | std::ios_base::binary)
        , as(as)
    {}

    const std::string& getDeviceName() {
        return filepath.string();
    }

//    void render(DecodeRange src) {
//        uint total_size = cast(uint)as.total_samples * as.channels * (as.bps/8);
//        file.rawWrite("RIFF");
//        file.rawWrite(ntl!uint(total_size + 36));
//        file.rawWrite("WAVEfmt ");
//        file.rawWrite(ntl!uint(16));
//        file.rawWrite(ntl!ushort(1));
//        file.rawWrite(ntl!ushort(as.channels));
//        file.rawWrite(ntl!uint(as.sample_rate));
//        ushort q = as.channels * (as.bps/8);
//        file.rawWrite(ntl!uint(as.sample_rate * q));
//        file.rawWrite(ntl!ushort(q));
//        file.rawWrite(ntl!ushort(as.bps));
//        file.rawWrite("data");
//        file.rawWrite(ntl!uint(total_size));
//        file.flush();
//        stderr.writeln("Written header");

//        foreach(buf; src) {
//            file.rawWrite(buf[]);
//        }
//    }

    const boost::filesystem::path& filepath;
    std::ofstream file;
    AudioSpecs as;
};

}

#endif // MELOSIC_WAV_HPP
