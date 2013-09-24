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
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/file.hpp>
namespace io = boost::iostreams;
#include <boost/filesystem.hpp>

#include <melosic/common/error.hpp>
#include <melosic/common/stream.hpp>

namespace Melosic {

namespace IO {

typedef std::ios_base::openmode OpenMode;
class File : public BiDirectionalClosableSeekable {
public:
    static constexpr OpenMode defaultMode = std::ios_base::binary | std::ios_base::in;

    File() = default;

    explicit File(boost::filesystem::path filename, OpenMode mode_ = defaultMode)
        :
          impl(filename.string(), mode_),
          m_filename(std::move(filename)), m_mode(mode_)
    {
//        impl.exceptions(impl.failbit | impl.badbit);
//        open(mode());
    }

    virtual ~File() {
        impl.close();
    }

    const boost::filesystem::path& filename() const {
        return m_filename;
    }

    explicit operator bool() const {
        return impl.is_open();
    }

    void open(const boost::filesystem::path& filename, OpenMode mode = defaultMode) {
        try {
            impl.close();
            m_filename = filename;
            impl.open(m_filename.string(), mode);
        }
        catch(...) {
            BOOST_THROW_EXCEPTION(FileOpenException() << ErrorTag::FilePath(m_filename)
                                  << boost::errinfo_nested_exception(boost::current_exception()));
        }
    }

    void open(OpenMode mode = defaultMode) {
        open(m_filename, mode);
    }

    std::streamsize read(char * s, std::streamsize n) override {
        try {
            return io::read(impl, s, n);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileReadException() << ErrorTag::FilePath(m_filename)
                                  << boost::errinfo_nested_exception(boost::current_exception()));
        }
    }

    std::streamsize write(const char * s, std::streamsize n) override {
        try {
            return io::write(impl, s, n);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileWriteException() << ErrorTag::FilePath(m_filename)
                                  << boost::errinfo_nested_exception(boost::current_exception()));
        }
    }

    std::streampos seekg(std::streamoff off, std::ios_base::seekdir way) override {
        try {
            return io::seek(impl, off, way, std::ios_base::in);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileSeekException() << ErrorTag::FilePath(m_filename)
                                  << boost::errinfo_nested_exception(boost::current_exception()));
        }
    }

    std::streampos seekp(std::streamoff off, std::ios_base::seekdir way) override {
        try {
            return io::seek(impl, off, way, std::ios_base::out);
        }
        catch(std::ios_base::failure& e) {
            BOOST_THROW_EXCEPTION(FileSeekException() << ErrorTag::FilePath(m_filename)
                                  << boost::errinfo_nested_exception(boost::current_exception()));
        }
    }

    void close() override {
        if(isOpen())
            impl.close();
    }

    bool isOpen() const override {
        return impl.is_open();
    }

    void reOpen() override {
        open(m_mode);
    }

    std::ios_base::openmode mode() const {
        return m_mode;
    }

    void clear() {
//        impl.clear();
    }

private:
//    io::stream_buffer<io::file> impl;
    io::file_descriptor impl;
//    io::file impl;
//    std::fstream impl;
    boost::filesystem::path m_filename;
    std::ios_base::openmode m_mode;
};

} // IO
} // Melosic

#endif // MELOSIC_FILE_HPP
