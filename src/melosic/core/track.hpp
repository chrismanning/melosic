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

#include <melosic/common/stream.hpp>
#include <melosic/core/inputmanager.hpp>
#include <melosic/managers/input/pluginterface.hpp>

namespace Melosic {

class Track : public Input::ISource, public IO::Closable
{
public:
    typedef std::multimap<std::string, std::string> TagsType;

    Track(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory);
    Track(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory, std::chrono::milliseconds offset);
    virtual ~Track();
    Track::TagsType& getTags();
    virtual std::streamsize do_read(char * s, std::streamsize n);
    virtual std::streampos do_seekg(std::streamoff off, std::ios_base::seekdir way);
    virtual void do_close();
    virtual bool do_isOpen();
    virtual void do_reOpen();
    virtual void seek(std::chrono::milliseconds dur);
    virtual std::chrono::milliseconds tell();
    virtual Melosic::AudioSpecs& getAudioSpecs();
    virtual explicit operator bool();
private:
    class impl;
    std::shared_ptr<impl> pimpl;
};

}

#endif // MELOSIC_TRACK_HPP
