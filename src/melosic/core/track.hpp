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

#include <boost/filesystem/path.hpp>

#include <melosic/common/common.hpp>
#include <melosic/common/stream.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/melin/decoder.hpp>

namespace Melosic {
namespace Core {

class MELOSIC_EXPORT Track : public Input::Playable, public IO::Closable {
    explicit Track(boost::filesystem::path filename,
                   Decoder::Factory,
                   chrono::milliseconds start = 0ms,
                   chrono::milliseconds end = 0ms);
    friend class Decoder::Manager;

public:
    typedef char char_type;

    virtual ~Track();

    bool operator==(const Track&) const;

    void setTimePoints(chrono::milliseconds start,
                       chrono::milliseconds end);

    virtual void reset();
    MELOSIC_EXPORT virtual void seek(chrono::milliseconds dur);
    MELOSIC_EXPORT virtual chrono::milliseconds tell();
    MELOSIC_EXPORT virtual chrono::milliseconds duration() const;
    MELOSIC_EXPORT virtual Melosic::AudioSpecs& getAudioSpecs();
    MELOSIC_EXPORT virtual const Melosic::AudioSpecs& getAudioSpecs() const;
    MELOSIC_EXPORT std::string getTag(const std::string& key) const;
    MELOSIC_EXPORT virtual explicit operator bool();
    virtual std::streamsize read(char * s, std::streamsize n);
    virtual void close();
    virtual bool isOpen() const;
    virtual void reOpen();
    MELOSIC_EXPORT const std::string& sourceName() const;

    void reloadTags();
    void reloadDecoder();

    static void setDecoderManager(const Decoder::Manager*) noexcept;

private:
    class impl;
    std::shared_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_TRACK_HPP
