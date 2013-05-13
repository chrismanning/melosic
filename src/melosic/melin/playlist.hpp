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

#include <boost/range/iterator_range.hpp>
#include <boost/container/stable_vector.hpp>

namespace Melosic {

namespace Core {
class Playlist;
}
namespace Slots {
class Manager;
}
namespace Decoder {
class Manager;
}

namespace Playlist {

typedef std::shared_ptr<Core::Playlist> PlaylistType;

class Manager {
public:
    typedef boost::container::stable_vector<PlaylistType> list;
    typedef list::iterator iterator;
    typedef list::const_iterator const_iterator;
    typedef boost::iterator_range<iterator> Range;
    typedef boost::iterator_range<const_iterator> ConstRange;

    Manager(Slots::Manager&, Decoder::Manager&);
    ~Manager();

    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    Range range() const;
    Range::iterator insert(Range::iterator pos, const std::string& name);
    Range::iterator insert(Range::iterator pos, int count);
    void erase(Range r);
    int count() const;
    bool empty() const;
    PlaylistType currentPlaylist() const;
    void setCurrent(Range::iterator) const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Playlist
} // namespace Melosic

#endif // MELOSIC_PLAYLIST_MANAGER_HPP
