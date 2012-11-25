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

#include <ios>
#include <boost/iostreams/categories.hpp>
namespace io = boost::iostreams;

namespace Melosic {

namespace IO {

class Device {
public:
    typedef char char_type;
    virtual ~Device() {}
};

class Closable : Device {
public:
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void reOpen() = 0;
};

class Source : public Device {
public:
    typedef io::source_tag category;
    virtual std::streamsize read(char * s, std::streamsize n) = 0;
};

class Sink : public Device {
public:
    typedef io::sink_tag category;
    virtual std::streamsize write(const char * s, std::streamsize n) = 0;
};

class BiDirectional : virtual public Source, virtual public Sink {
public:
    typedef io::bidirectional_device_tag category;
};

class SeekableSource : public Source {
public:
    typedef io::input_seekable category;
    virtual std::streampos seekg(std::streamoff off, std::ios_base::seekdir way) = 0;

    std::streamoff tellg() {
        return seekg(0, std::ios_base::cur);
    }
};

class SeekableClosableSource : public SeekableSource, public Closable {
public:
    struct category : io::closable_tag, io::input_seekable {};
};

class SeekableSink : public Sink {
public:
    typedef io::output_seekable category;
    virtual std::streampos seekp(std::streamoff off, std::ios_base::seekdir way) = 0;

    std::streamoff tellp() {
        return seekp(0, std::ios_base::cur);
    }
};

class BiDirectionalSeekable : virtual public SeekableSource, virtual public SeekableSink {
public:
    typedef io::bidirectional_seekable category;

    std::streampos seek(std::streamoff off,
                        std::ios_base::seekdir way,
                        std::ios_base::openmode which)
    {
        std::streampos r;
        if(which & std::ios_base::in) {
            r = seekg(off, way);
        }
        if(which & std::ios_base::out) {
            r = seekp(off, way);
        }
        return r;
    }
};

class BiDirectionalClosableSeekable : virtual public BiDirectionalSeekable, virtual public Closable {
public:
    struct category : io::closable_tag, io::bidirectional_seekable {};
};

}
}

#endif // MELOSIC_STREAM_HPP
