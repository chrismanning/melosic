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

#include <melosic/common/common.hpp>
#include <melosic/common/stream.hpp>
#include <melosic/melin/input.hpp>

namespace Melosic {

namespace Decoder {
class Manager;
}

class Track : public Input::Playable, public IO::Closable {
public:
    Track(Melosic::Decoder::Manager& decman,
          const std::string& filename,
          chrono::milliseconds start = chrono::milliseconds(0),
          chrono::milliseconds end = chrono::milliseconds(0));

    virtual ~Track();
    Track(const Track&);
    Track& operator=(const Track&);

    virtual void reset();
    virtual void seek(chrono::milliseconds dur);
    virtual chrono::milliseconds tell();
    virtual chrono::milliseconds duration() const;
    virtual Melosic::AudioSpecs& getAudioSpecs();
    virtual const Melosic::AudioSpecs& getAudioSpecs() const;
    std::string getTag(const std::string& key) const;
    virtual explicit operator bool();
    virtual std::streamsize read(char * s, std::streamsize n);
    virtual void close();
    virtual bool isOpen() const;
    virtual void reOpen();
    const std::string& sourceName() const;

    void reloadTags();
    void reloadDecoder();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}

#endif // MELOSIC_TRACK_HPP
