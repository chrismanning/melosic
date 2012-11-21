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

#ifndef MELOSIC_TRACK_HPP
#define MELOSIC_TRACK_HPP

#include <memory>
#include <ios>
#include <map>
#include <chrono>
#include <boost/shared_ptr.hpp>

#include <melosic/common/stream.hpp>
#include <melosic/managers/input/pluginterface.hpp>

namespace Melosic {

class Track : public Input::Source, public IO::Closable
{
public:
    typedef std::multimap<std::string, std::string> TagsType;

    Track(const std::string& filename,
          std::chrono::milliseconds start = std::chrono::milliseconds(0),
          std::chrono::milliseconds end = std::chrono::milliseconds(0));

    virtual ~Track();
    Track(const Track&);
    void operator=(const Track&);

    Track::TagsType& getTags();
    virtual void reset();
    virtual void seek(std::chrono::milliseconds dur);
    virtual std::chrono::milliseconds tell();
    virtual std::chrono::milliseconds duration() const;
    virtual Melosic::AudioSpecs& getAudioSpecs();
    virtual explicit operator bool();
    virtual std::streamsize read(char * s, std::streamsize n);
    virtual void close();
    virtual bool isOpen();
    virtual void reOpen();
    const std::string& sourceName() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}

#endif // MELOSIC_TRACK_HPP
