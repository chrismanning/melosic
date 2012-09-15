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

#include <string>
#include <fstream>
typedef std::ios_base::openmode openmode;
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
namespace io = boost::iostreams;

#include <melosic/common/error.hpp>
#include <melosic/common/stream.hpp>

namespace Melosic {

namespace IO {

class File : public BiDirectionalClosableSeekable {
public:
    File(const std::string& filename, const openmode mode = mode_)
        : impl(filename, mode), filename_(filename)
    {
        enforceEx<Exception>((bool)(*this), (filename_ + ": could not open").c_str());
    }

    virtual ~File() {}

    const std::string& filename() const {
        return filename_;
    }

    explicit operator bool() {
        return impl.is_open();
    }

    void open(const openmode mode = std::ios_base::binary | std::ios_base::in | std::ios_base::out)
    {
        impl.open(filename_, mode);
    }

private:
    auto static const mode_ = std::ios_base::binary | std::ios_base::in | std::ios_base::out;
//    io::stream_buffer<io::file> impl;
//    io::file_descriptor impl;
    std::fstream impl;
    std::string filename_;

    virtual std::streamsize do_read(char * s, std::streamsize n) {
        return io::read(impl, s, n);
    }

    virtual std::streamsize do_write(const char * s, std::streamsize n) {
        return io::write(impl, s, n);
    }

    virtual std::streampos do_seekg(std::streamoff off, std::ios_base::seekdir way) {
        return io::seek(impl, off, way, std::ios_base::in);
    }

    virtual std::streampos do_seekp(std::streamoff off, std::ios_base::seekdir way) {
        return io::seek(impl, off, way, std::ios_base::out);
    }

    virtual void do_close() {
        impl.close();
    }

    virtual bool do_isOpen() {
        return bool(*this);
    }

    virtual void do_reOpen() {
        open();
    }
};

struct IOException : Exception {
    IOException(const File& fs, const char * msg) : Exception(msg), fs(fs) {}
private:
    const File& fs;
};

} // IO
} // Melosic

#endif // MELOSIC_FILE_HPP
