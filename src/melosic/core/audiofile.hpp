/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#ifndef MELOSIC_AUDIOFILE_HPP
#define MELOSIC_AUDIOFILE_HPP

#include <memory>
#include <chrono>
namespace chrono = std::chrono;

#include <boost/filesystem/path.hpp>
#include <boost/thread/synchronized_value.hpp>

#include <jbson/document_fwd.hpp>

namespace Melosic {
namespace Decoder {
class Manager;
}
namespace Library {
class Manager;
}
namespace Core {

class Track;

struct AudioFile {
    AudioFile() = default;

    const boost::filesystem::path& filePath() const;
    size_t trackCount() const;
    size_t fileSize() const;

    enum class Type {
        Unsupported,
        SingleTrack,
        MultiTrack, //embedded cue
        Playlist,   //m3u etc.
        Cue         //external cue
    };

    Type type() const;

    boost::synchronized_value<std::vector<Track>>& tracks();
    const boost::synchronized_value<std::vector<Track>>& tracks() const;

private:
    explicit AudioFile(const boost::filesystem::path&);
    friend class FileCache;
    friend class Library::Manager;

    struct impl;
    std::shared_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_AUDIOFILE_HPP
