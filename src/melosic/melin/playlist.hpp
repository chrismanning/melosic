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

#ifndef MELOSIC_PLAYLIST_MANAGER_HPP
#define MELOSIC_PLAYLIST_MANAGER_HPP

#include <memory>

#include <boost/optional/optional_fwd.hpp>

#include <melosic/common/common.hpp>
#include <melosic/melin/playlist_signals.hpp>

namespace Melosic {

namespace Core {
class Playlist;
}
namespace Decoder {
class Manager;
}

namespace Playlist {

class Manager {
public:
    typedef int size_type;

    explicit Manager(Decoder::Manager&);
    ~Manager();

    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    MELOSIC_EXPORT boost::optional<Core::Playlist> insert(size_type pos, std::string name);
    MELOSIC_EXPORT size_type insert(size_type pos, int size);
    MELOSIC_EXPORT void erase(size_type, size_type);

    MELOSIC_EXPORT boost::optional<Core::Playlist> getPlaylist(size_type pos);
    MELOSIC_EXPORT const boost::optional<Core::Playlist> getPlaylist(size_type pos) const;

    MELOSIC_EXPORT size_type size() const;
    MELOSIC_EXPORT bool empty() const;
    MELOSIC_EXPORT boost::optional<Core::Playlist> currentPlaylist() const;
    MELOSIC_EXPORT void setCurrent(boost::optional<Core::Playlist>) const;
    MELOSIC_EXPORT void setCurrent(size_type) const;

    MELOSIC_EXPORT Signals::Playlist::CurrentPlaylistChanged& getCurrentPlaylistChangedSignal() const;
    MELOSIC_EXPORT Signals::Playlist::PlaylistAdded& getPlaylistAddedSignal() const;
    MELOSIC_EXPORT Signals::Playlist::PlaylistRemoved& getPlaylistRemovedSignal() const;

    MELOSIC_EXPORT Signals::Playlist::CurrentTrackChanged& getTrackChangedSignal() const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Playlist
} // namespace Melosic

#endif // MELOSIC_PLAYLIST_MANAGER_HPP
