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

#ifndef MELOSIC_STREAM_HPP
#define MELOSIC_STREAM_HPP

#include <iosfwd>
#include <boost/iostreams/categories.hpp>
namespace io = boost::iostreams;

namespace Melosic {

namespace IO {

class Device {
public:
    typedef char char_type;
    virtual ~Device() {}
};

class Source : public Device {
public:
    typedef io::source_tag category;

    std::streamsize read(char * s, std::streamsize n) {
        return do_read(s, n);
    }

private:
    virtual std::streamsize do_read(char * s, std::streamsize n) = 0;
};

class Sink : public Device {
public:
    typedef io::sink_tag category;

    std::streamsize write(const char * s, std::streamsize n) {
        return do_write(s, n);
    }

private:
    virtual std::streamsize do_write(const char * s, std::streamsize n) = 0;
};

class BiDirectional : virtual public Source, virtual public Sink {
public:
    typedef io::bidirectional_device_tag category;
};

class BiDirectionalSeekable : public BiDirectional {
public:
    typedef io::bidirectional_seekable category;

    std::streampos seek(std::streamoff off,
                        std::ios_base::seekdir way,
                        std::ios_base::openmode which)
    {
        return do_seek(off, way, which);
    }

private:
    virtual std::streampos do_seek(std::streamoff off,
                                   std::ios_base::seekdir way,
                                   std::ios_base::openmode which) = 0;
};

class SeekableSink : public Sink {
    typedef io::input_seekable category;
};

}
}

#endif // MELOSIC_STREAM_HPP
