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
#include <mutex>
using std::mutex;
using lock_guard = std::lock_guard<mutex>;
using unique_lock = std::unique_lock<mutex>;

#include <boost/range/algorithm_ext.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/adaptors.hpp>
using namespace boost::adaptors;

#include <melosic/core/playlist.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/common/string.hpp>
#include <melosic/melin/playlist_signals.hpp>

#include "playlist.hpp"

namespace Melosic {
namespace Playlist {

struct PlaylistChanged : Signals::Signal<Signals::Playlist::PlaylistChanged> {
    using Super::Signal;
};

class Manager::impl {
public:
    impl(Decoder::Manager& decman) :
        decman(decman)
    {}

    Manager::Range::iterator insert(Manager::Range::iterator pos, const std::string& name) {
        lock_guard l(mu);
        return playlists.insert(pos, std::make_shared<Core::Playlist>(name, decman));
    }

    Manager::Range::iterator insert(Manager::Range::iterator pos, int count_) {
        lock_guard l(mu);
        if(count_ == 0)
            return pos;
        for(int beg = std::distance(playlists.begin(), pos), i = 0; i < count_; i++) {
            playlists.insert(pos + i, std::make_shared<Core::Playlist>("Playlist "s + std::to_string(i+beg), decman));
        }
        return ++pos;
    }

    void erase(Manager::Range r) {
        lock_guard l(mu);
        boost::range::erase(playlists, r);
    }

    int count() {
        lock_guard l(mu);
        return playlists.size();
    }

    bool empty() {
        lock_guard l(mu);
        return playlists.empty();
    }

    Manager::iterator begin() {
        lock_guard l(mu);
        return playlists.begin();
    }

    Manager::iterator end() {
        lock_guard l(mu);
        return playlists.end();
    }

    PlaylistType& front() {
        lock_guard l(mu);
        return playlists.front();
    }

    PlaylistType& back() {
        lock_guard l(mu);
        return playlists.back();
    }

    PlaylistType currentPlaylist() {
        unique_lock l(mu);

        if(current)
            return current;

        if(!playlists.empty()) {
            l.unlock();
            setCurrent(front());
        }
        else {
            l.unlock();
            setCurrent(*insert(begin(), "Default"));
        }
        assert(current);
        return current;
    }

    void setCurrent(PlaylistType p) {
        lock_guard l(mu);
        current = p;
        playlistChanged(p);
    }

    Signals::Playlist::PlaylistChanged& getPlaylistChangedSignal() {
        return playlistChanged;
    }

private:
    mutex mu;

    Decoder::Manager& decman;
    PlaylistChanged playlistChanged;
    PlaylistType current;
    Manager::list playlists;

    friend class Manager;
};

Manager::Manager(Decoder::Manager& decman) : pimpl(new impl(decman)) {}

Manager::~Manager() {}

Manager::Range::iterator Manager::insert(Range::iterator pos, const std::string& name) {
    return pimpl->insert(pos, name);
}

Manager::Range::iterator Manager::insert(Range::iterator pos, int count) {
    return pimpl->insert(pos, count);
}

void Manager::erase(Manager::Range r) {
    pimpl->erase(r);
}

int Manager::count() const {
    return pimpl->count();
}

bool Manager::empty() const {
    return pimpl->empty();
}

PlaylistType Manager::currentPlaylist() const {
    return pimpl->currentPlaylist();
}

void Manager::setCurrent(PlaylistType p) const {
    pimpl->setCurrent(p);
}

Signals::Playlist::PlaylistChanged& Manager::getPlaylistChangedSignal() const {
    return pimpl->getPlaylistChangedSignal();
}

Signals::Playlist::TrackChanged& Manager::getTrackChangedSignal() const {
    return Core::Playlist::getTrackChangedSignal();
}

Manager::Range Manager::range() const {
    return pimpl->playlists;
}

} // namespace Playlist
} // namespace Melosic
