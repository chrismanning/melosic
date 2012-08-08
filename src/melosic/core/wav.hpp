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
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
namespace io = boost::iostreams;
#include <fstream>

#include <melosic/common/common.hpp>
#include <melosic/managers/output/pluginterface.hpp>

namespace Melosic {
namespace Output {

class WaveFile : public IFileSink {
public:
    WaveFile(const boost::filesystem::path& filename, AudioSpecs as)
        : filepath(absolute(filename))
        , file(filepath.string(), std::ios_base::out | std::ios_base::binary)
        , as(as)
    {
        auto total_size = (uint32_t)as.total_samples * as.channels * (as.bps/8);
        auto header = new char[44];
        auto tmp = header;

        strcpy(tmp, "RIFF"); tmp+=4;
        *(uint32_t*)tmp = total_size + 36; tmp+=4;
        strcpy(tmp, "WAVEfmt "); tmp+=8;
        *(uint32_t*)tmp = 16; tmp+=4;
        *(uint16_t*)tmp = 1; tmp+=2;
        *(uint16_t*)tmp = as.channels; tmp+=2;
        *(uint32_t*)tmp = as.sample_rate; tmp+=4;
        uint16_t q = as.channels * (as.bps/8);
        *(uint32_t*)tmp = as.sample_rate * q; tmp+=4;
        *(uint16_t*)tmp = q; tmp+=2;
        *(uint16_t*)tmp = as.bps; tmp+=2;
        strcpy(tmp, "data"); tmp+=4;
        *(uint32_t*)tmp = total_size; tmp+=4;

        io::write(file, header, 44);
    }

    virtual const std::string& getSinkName() {
        return filepath.string();
    }

    virtual std::streamsize write(const char * s, std::streamsize n) {
        return io::write(file, s, n);
    }

private:
    const boost::filesystem::path& filepath;
    io::file_descriptor_sink file;
    AudioSpecs as;
};

}
}

#endif // MELOSIC_WAV_HPP
