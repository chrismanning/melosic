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
#include <optional>

#include <boost/filesystem/path.hpp>

#include <melosic/common/common.hpp>
#include <melosic/common/stream.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/melin/decoder.hpp>

namespace Melosic {
namespace Core {

class MELOSIC_EXPORT Track {
    explicit Track(Decoder::Factory,
                   boost::filesystem::path filename,
                   chrono::milliseconds start = 0ms,
                   chrono::milliseconds end = 0ms);
    friend class Decoder::Manager;

public:
    typedef char char_type;
    typedef io::source_tag category;

    virtual ~Track();

    bool operator==(const Track&) const noexcept;

    void setTimePoints(chrono::milliseconds start, chrono::milliseconds end);

    std::streamsize read(char * s, std::streamsize n);
    void reset();
    void seek(chrono::milliseconds dur);
    void close();
    bool isOpen() const;
    void reOpen();

    chrono::milliseconds tell();
    chrono::milliseconds duration() const;
    Melosic::AudioSpecs& getAudioSpecs();
    const Melosic::AudioSpecs& getAudioSpecs() const;
    const std::string& sourceName() const;
    std::optional<std::string> getTag(const std::string& key) const;

    void reloadTags();
    bool taggable() const;
    void reloadDecoder();
    bool decodable() const;

    bool valid() const;
    explicit operator bool() const;
    Track clone() const;

private:
    class impl;
    std::shared_ptr<impl> pimpl;
    explicit Track(decltype(pimpl));
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_TRACK_HPP
