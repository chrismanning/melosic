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
#include <boost/filesystem.hpp>

#include <melosic/common/error.hpp>
#include <melosic/common/stream.hpp>

namespace Melosic {

namespace IO {

class File : public BiDirectionalClosableSeekable {
public:
    auto static const defaultMode = std::ios_base::binary | std::ios_base::in | std::ios_base::out;

    File(const boost::filesystem::path& filename, const openmode mode_ = defaultMode)
        : filename_(filename), mode_(mode_)
    {
        impl.exceptions(impl.failbit | impl.badbit);
        open(mode());
    }

    virtual ~File() {}

    const boost::filesystem::path& filename() const {
        return filename_;
    }

    explicit operator bool() const {
        return impl.is_open();
    }

    void open(const openmode mode = std::ios_base::binary | std::ios_base::in | std::ios_base::out)
    {
        try {
            impl.open(filename_.string(), mode);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileOpenException() << ErrorTag::FilePath(filename_));
        }
    }

    virtual std::streamsize read(char * s, std::streamsize n) {
        try {
            return io::read(impl, s, n);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileReadException() << ErrorTag::FilePath(filename_));
        }
    }

    virtual std::streamsize write(const char * s, std::streamsize n) {
        try {
            return io::write(impl, s, n);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileWriteException() << ErrorTag::FilePath(filename_));
        }
    }

    virtual std::streampos seekg(std::streamoff off, std::ios_base::seekdir way) {
        try {
            return io::seek(impl, off, way, std::ios_base::in);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileSeekException() << ErrorTag::FilePath(filename_));
        }
    }

    virtual std::streampos seekp(std::streamoff off, std::ios_base::seekdir way) {
        try {
            return io::seek(impl, off, way, std::ios_base::out);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileSeekException() << ErrorTag::FilePath(filename_));
        }
    }

    virtual void close() {
        if(isOpen())
            impl.close();
    }

    virtual bool isOpen() const {
        return impl.is_open();
    }

    virtual void reOpen() {
        open();
    }

    std::ios_base::openmode mode() const {
        return mode_;
    }

    void clear() {
        impl.clear();
    }

private:
//    io::stream_buffer<io::file> impl;
//    io::file_descriptor impl;
    std::fstream impl;
    boost::filesystem::path filename_;
    std::ios_base::openmode mode_;
};

} // IO
} // Melosic

#endif // MELOSIC_FILE_HPP
