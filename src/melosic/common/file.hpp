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

#ifndef MELOSIC_FILE_HPP
#define MELOSIC_FILE_HPP

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <type_traits>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file.hpp>
namespace io = boost::iostreams;

#include <melosic/common/error.hpp>

namespace Melosic {

namespace IO {

class File : public io::file {
public:
    File(std::string filename) : io::file(filename, mode), filename_(filename) {}

    void reopen() {
        if(!is_open()) {
            open(filename_, mode);
        }
    }

    const std::string& filename() {
        return filename_;
    }

private:
    auto static const mode = std::ios_base::binary | std::ios_base::in | std::ios_base::out;
    std::string filename_;
};

struct IOException : Exception {
    IOException(const File& fs, const char * msg) : Exception(msg), fs(fs) {}
private:
    const File& fs;
};

} // IO
} // Melosic

#endif // MELOSIC_FILE_HPP
