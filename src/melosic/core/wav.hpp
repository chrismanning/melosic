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

#include <boost/iostreams/write.hpp>
#include <boost/iostreams/device/file.hpp>
namespace io = boost::iostreams;
#include <fstream>

#include <melosic/common/common.hpp>
#include <melosic/managers/output/pluginterface.hpp>

namespace Melosic {
namespace Output {

class WaveFile : public FileSink {
public:
    WaveFile(IO::File& file, AudioSpecs as)
        : file(file)
        , as(as)
    {
        auto ex = as.bps % 8 || as.channels > 2;
        uint32_t total_size = as.total_samples * as.channels * (as.bps/8);
        auto header = new char[ex ? 68 : 44];
        memset(header, 0, ex ? 68 : 44);
        auto tmp = header;

        strcpy(tmp, "RIFF"); tmp+=4;
        *(uint32_t*)tmp = total_size;
        *tmp+= ex ? 60 : 36; tmp+=4;
        strcpy(tmp, "WAVEfmt "); tmp+=8;
        *(uint32_t*)tmp = ex ? 40 : 16; tmp+=4;
        *(uint16_t*)tmp = ex ? 0xFFFE : 0x0001; tmp+=2;
        *(uint16_t*)tmp = as.channels; tmp+=2;
        *(uint32_t*)tmp = as.sample_rate; tmp+=4;
        uint16_t q = as.channels * (as.bps/8);
        *(uint32_t*)tmp = as.sample_rate * q; tmp+=4;
        *(uint16_t*)tmp = q; tmp+=2;
        *(uint16_t*)tmp = as.bps; tmp+=2;

        if(ex) {
            *(uint16_t*)tmp = 22; tmp+=2;
            *(uint16_t*)tmp = as.bps; tmp+=2;

            switch(as.channels) {
                case 1:
                    *(uint32_t*)tmp = 0x0001; tmp+=4;
                    break;
                case 2:
                    *(uint32_t*)tmp = 0x0003; tmp+=4;
                    break;
                case 3:
                    *(uint32_t*)tmp = 0x0007; tmp+=4;
                    break;
                case 4:
                    *(uint32_t*)tmp = 0x0033; tmp+=4;
                    break;
                case 5:
                    *(uint32_t*)tmp = 0x0607; tmp+=4;
                    break;
                case 6:
                    *(uint32_t*)tmp = 0x060f; tmp+=4;
                    break;
                default:
                    *(uint32_t*)tmp = 0x0000; tmp+=4;
                    break;
            }

            memcpy(tmp, "\x01\x00\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71", 16); tmp+=16;
        }

        strcpy(tmp, "data"); tmp+=4;
        *(uint32_t*)tmp = total_size; tmp+=4;

        io::write(file, header, ex ? 68 : 44);
    }

    virtual const std::string& getSinkName() {
        return file.filename().string();
    }

    virtual std::streamsize write(const char * s, std::streamsize n) {
        return io::write(file, s, n);
    }

private:
    IO::File& file;
    AudioSpecs as;
};

}
}

#endif // MELOSIC_WAV_HPP
