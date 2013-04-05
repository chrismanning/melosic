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

#include <list>
#include <string>

#include <boost/range/algorithm_ext.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/core/playlist.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/common/string.hpp>

#include "playlist.hpp"

namespace Melosic {
namespace Playlist {

class Manager::impl {
public:
    impl(Slots::Manager& slotman, Decoder::Manager& decman)
        : slotman(slotman),
          decman(decman),
          playlistChanged(slotman.get<Signals::Playlist::PlaylistChanged>())
    {}

    Manager::Range::iterator insert(Manager::Range::iterator pos, const std::string& name) {
        return playlists.insert(pos, std::make_shared<Core::Playlist>(name, slotman, decman));
    }

    Manager::Range::iterator insert(Manager::Range::iterator pos, int count_) {
        if(count_ == 0)
            return pos;
        std::function<PlaylistType(int)> fun = [this] (int c) {
            return std::make_shared<Core::Playlist>("Playlist "_str + to_string(c), slotman, decman);
        };
        boost::range::insert(playlists, pos, boost::irange(count(), count()+count_) | transformed(fun));
        return ++pos;
    }

    void erase(Manager::Range r) {
        boost::range::erase(playlists, r);
    }

    int count() {
        return playlists.size();
    }

    bool empty() {
        return playlists.empty();
    }

    Manager::Range::iterator begin() {
        return playlists.begin();
    }

    Manager::Range::iterator end() {
        return playlists.end();
    }

    PlaylistType& front() {
        return playlists.front();
    }
    PlaylistType& back() {
        return playlists.back();
    }

private:
    Slots::Manager& slotman;
    Decoder::Manager& decman;
    Signals::Playlist::PlaylistChanged& playlistChanged;
    Manager::Range::iterator current;
    Manager::list playlists;

    friend class Manager;
};

Manager::Manager(Slots::Manager& slotman, Decoder::Manager& decman) : pimpl(new impl(slotman, decman)) {}

Manager::~Manager() {}

Manager::Range::iterator Manager::insert(Range::iterator pos, const std::string& name) {
    return pimpl->insert(pos, name);
}

Manager::Range::iterator Manager::insert(Range::iterator pos, int count) {
    return pimpl->insert(pos, count);
}

void Manager::erase(Manager::Range r) {
    pimpl->erase(std::move(r));
}

int Manager::count() const {
    return pimpl->count();
}

bool Manager::empty() const {
    return pimpl->empty();
}

Manager::Range::iterator Manager::current() const {
    if(pimpl->current != pimpl->end())
        return pimpl->current;

    if(!empty())
        setCurrent(pimpl->begin());
    else
        setCurrent(const_cast<Manager*>(this)->insert(pimpl->begin(), 1));
    return current();
}

void Manager::setCurrent(Range::iterator it) const {
    pimpl->current = it;
    pimpl->playlistChanged(*it);
}

Manager::Range Manager::range() const {
    return pimpl->playlists;
}

} // namespace Playlist
} // namespace Melosic
