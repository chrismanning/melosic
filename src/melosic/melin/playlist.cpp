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

#include <boost/thread/shared_mutex.hpp>
using mutex = boost::shared_mutex;
using lock_guard = std::lock_guard<mutex>;
using unique_lock = std::unique_lock<mutex>;
using shared_lock = boost::shared_lock<mutex>;
#include <boost/scope_exit.hpp>
#include <boost/container/stable_vector.hpp>

#include <melosic/core/playlist.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/common/string.hpp>
#include <melosic/melin/playlist_signals.hpp>
#include <melosic/melin/logging.hpp>

#include "playlist.hpp"

namespace Melosic {
namespace Playlist {

struct CurrentPlaylistChanged : Signals::Signal<Signals::Playlist::CurrentPlaylistChanged> {
    using Super::Signal;
};

struct PlaylistAdded : Signals::Signal<Signals::Playlist::PlaylistAdded> {
    using Super::Signal;
};

struct PlaylistRemoved : Signals::Signal<Signals::Playlist::PlaylistRemoved> {
    using Super::Signal;
};

class Manager::impl {
public:
    impl(Decoder::Manager& decman) : decman(decman)
    {}

    std::optional<Core::Playlist> insert(size_type pos, const std::string& name, unique_lock& l) {
        assert(pos >= 0);
        TRACE_LOG(logject) << "inserting playlist \"" << name << "\" at position " << pos;
        auto it = playlists.emplace(std::next(std::begin(playlists), pos), decman, name);
        assert(!playlists.empty());
        playlistAdded(*it, l);
        if(size() == 1)
            setCurrent(*it, l);
        return *it;
    }

    size_type insert(size_type pos, size_type count_, unique_lock& l) {
        assert(pos >= 0);
        assert(count_ >= 0);
        if(count_ == 0)
            return 0;
        TRACE_LOG(logject) << "inserting " << count_ << " playlists at position " << pos;

        size_type s{count_};
        for(int i = 0; i < count_; i++, pos++)
            if(!insert(pos, "Playlist "s + std::to_string(pos), l))
                s--;
        return s;
    }

    void erase(size_type pos, unique_lock& l) {
        assert(pos >= 0);
        TRACE_LOG(logject) << "erasing playlist at position " << pos;
        auto old = playlists[pos];
        playlists.erase(std::next(std::begin(playlists), pos));
        playlistRemoved(old, l);
    }

    void erase(size_type start, size_type end, unique_lock& l) {
        assert(end > start);
        TRACE_LOG(logject) << "erasing playlists from positions " << start << " to " << end;
        for(size_type i = --end; i >= start; i--)
            erase(i, l);
    }

    size_type size() {
        return playlists.size();
    }

    bool empty() {
        return playlists.empty();
    }

    std::optional<Core::Playlist> currentPlaylist() {
        return current;
    }

    void setCurrent(std::optional<Core::Playlist> p, unique_lock& l) {
        TRACE_LOG(logject) << "setting current playlist " << !!p;
        if(p == current)
            return;
        current = p;
        currentPlaylistChanged(current, l);
    }

    void setCurrent(size_type p, unique_lock& l) {
        TRACE_LOG(logject) << "setting current playlist to position " << p;
        current = playlists[p];
        currentPlaylistChanged(current, l);
    }

    void currentPlaylistChanged(std::optional<Core::Playlist> p, unique_lock& l) {
        l.unlock();
        BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
        currentPlaylistChangedSignal(p);
    }

    void playlistAdded(std::optional<Core::Playlist> p, unique_lock& l) {
        l.unlock();
        BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
        playlistAddedSignal(p);
    }

    void playlistRemoved(std::optional<Core::Playlist> p, unique_lock& l) {
        l.unlock();
        BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
        playlistRemovedSignal(p);
    }

private:
    Decoder::Manager& decman;

    mutex mu;
    Logger::Logger logject{logging::keywords::channel = "PlaylistManagerModel"};

    CurrentPlaylistChanged currentPlaylistChangedSignal;
    PlaylistAdded playlistAddedSignal;
    PlaylistRemoved playlistRemovedSignal;
    std::optional<Core::Playlist> current;
    boost::container::stable_vector<Core::Playlist> playlists;

    friend class Manager;
};

Manager::Manager(Decoder::Manager& decman) : pimpl(new impl(decman)) {}

Manager::~Manager() {}

std::optional<Core::Playlist> Manager::insert(size_type pos, std::string name) {
    unique_lock l(pimpl->mu);
    return pimpl->insert(pos, name, l);
}

Manager::size_type Manager::insert(size_type pos, int count) {
    unique_lock l(pimpl->mu);
    return pimpl->insert(pos, count, l);
}

void Manager::erase(size_type start, size_type end) {
    unique_lock l(pimpl->mu);
    pimpl->erase(start, end, l);
}

std::optional<Core::Playlist> Manager::getPlaylist(size_type pos) {
    assert(pos >= 0);
    shared_lock l(pimpl->mu);
    if(pos >= size()) {
        TRACE_LOG(pimpl->logject) << "no playlist at position " << pos;
        return {};
    }
    TRACE_LOG(pimpl->logject) << "getting playlists at position " << pos;
    return pimpl->playlists[pos];
}

const std::optional<Core::Playlist> Manager::getPlaylist(size_type pos) const {
    return const_cast<Manager*>(this)->getPlaylist(pos);
}

int Manager::size() const {
    shared_lock l(pimpl->mu);
    return pimpl->size();
}

bool Manager::empty() const {
    shared_lock l(pimpl->mu);
    return pimpl->empty();
}

std::optional<Core::Playlist> Manager::currentPlaylist() const {
    shared_lock l(pimpl->mu);
    return pimpl->currentPlaylist();
}

void Manager::setCurrent(std::optional<Core::Playlist> p) const {
    unique_lock l(pimpl->mu);
    pimpl->setCurrent(p, l);
}

void Manager::setCurrent(size_type idx) const {
    unique_lock l(pimpl->mu);
    pimpl->setCurrent(idx, l);
}

Signals::Playlist::CurrentPlaylistChanged& Manager::getCurrentPlaylistChangedSignal() const {
    return pimpl->currentPlaylistChangedSignal;
}

Signals::Playlist::PlaylistAdded& Manager::getPlaylistAddedSignal() const {
    return pimpl->playlistAddedSignal;
}

Signals::Playlist::PlaylistRemoved& Manager::getPlaylistRemovedSignal() const {
    return pimpl->playlistRemovedSignal;
}

Signals::Playlist::CurrentTrackChanged& Manager::getTrackChangedSignal() const {
    return Core::Playlist::getCurrentTrackChangedSignal();
}

} // namespace Playlist
} // namespace Melosic
