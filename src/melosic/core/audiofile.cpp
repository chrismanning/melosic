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

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/range/iterator_range.hpp>

#include <melosic/core/track.hpp>

#include "audiofile.hpp"

namespace Melosic {
namespace Core {

struct AudioFile::impl {
    explicit impl(const fs::path& p) : m_path(fs::canonical(p)) {}

    fs::path m_path;
    boost::synchronized_value<std::vector<Track>> m_tracks;
};

AudioFile::AudioFile(const fs::path& p) : pimpl(std::make_shared<impl>(p)) {}

const fs::path& AudioFile::filePath() const {
    return pimpl->m_path;
}

size_t AudioFile::trackCount() const {
    return pimpl->m_tracks->size();
}

size_t AudioFile::fileSize() const {
    return fs::file_size(pimpl->m_path);
}

AudioFile::Type AudioFile::type() const {
    return Type::Unsupported;
}

boost::synchronized_value<std::vector<Track>>& AudioFile::tracks() {
    return pimpl->m_tracks;
}

const boost::synchronized_value<std::vector<Track>>& AudioFile::tracks() const {
    return pimpl->m_tracks;
}

} // namespace Core
} // namespace Melosic
