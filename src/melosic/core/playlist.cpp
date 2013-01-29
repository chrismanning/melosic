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

#include <algorithm>
#include <numeric>
#include <thread>
using std::mutex; using std::unique_lock; using std::lock_guard;
#include <deque>

#include <boost/container/stable_vector.hpp>

#include <melosic/core/track.hpp>
#include <melosic/core/logging.hpp>
#include "playlist.hpp"

namespace Melosic {

class Playlist::impl {
public:
    impl() : logject(boost::log::keywords::channel = "Playlist"), current_track_(begin()) {}

    //IO
    std::streamsize read(char* s, std::streamsize n) {
        if(currentTrack() != end()) {
            auto r = currentTrack()->read(s, n);
            if(r < n) {
                if(r < 0)
                    r = 0;
                currentTrack()->close();
                next();
                if(currentTrack() != end()) {
                    currentTrack()->reOpen();
                    currentTrack()->reset();
                    r += currentTrack()->read(s + r, n - r);
                }
            }
            return r;
        }
        return -1;
    }

    void seek(chrono::milliseconds dur) {
        if(currentTrack() != end()) {
            currentTrack()->seek(dur);
        }
    }

    //playlist controls
    chrono::milliseconds duration() {
        return std::accumulate(begin(), end(), chrono::milliseconds(0),
                               [&](chrono::milliseconds a, Track& b) { return a + b.duration();});
    }

    void previous() {
        if(currentTrack() == begin()) {
            seek(chrono::milliseconds(0));
        }
        else if(size() >= 1) {
            lock_guard<Mutex> l(mu);
            --current_track_;
        }
        trackChanged(*currentTrack(), true);
    }

    void next() {
        if(currentTrack() != end()) {
            lock_guard<Mutex> l(mu);
            ++current_track_;
        }
        trackChanged(*currentTrack(), currentTrack() != end());
    }

    Playlist::iterator& currentTrack() {
        lock_guard<Mutex> l(mu);
        return current_track_;
    }

    //element access
    Playlist::reference front() {
        lock_guard<Mutex> l(mu);
        return tracks.front();
    }

    Playlist::reference back() {
        lock_guard<Mutex> l(mu);
        return tracks.back();
    }

    //iterators
    Playlist::iterator begin() {
        lock_guard<Mutex> l(mu);
        return tracks.begin();
    }

    Playlist::iterator end() {
        lock_guard<Mutex>l(mu);
        return tracks.end();
    }

    //capacity
    bool empty() {
        lock_guard<Mutex> l(mu);
        return tracks.empty();
    }

    Playlist::size_type size() {
        lock_guard<Mutex> l(mu);
        return tracks.size();
    }

    Playlist::size_type max_size() const {
        return std::numeric_limits<Playlist::size_type>::max();
    }

    Playlist::iterator insert(Playlist::iterator pos, Playlist::value_type&& value) {
        unique_lock<Mutex> l(mu);
        auto r = tracks.insert(pos.cast_to<list_type::iterator>(), value);
        l.unlock();
        if(size() == 1) {
            current_track_ = r;
            trackChanged(*current_track_, currentTrack() != end());
        }
        return r;
    }

    void insert(Playlist::iterator pos, Playlist::iterator first, iterator last) {
        unique_lock<Mutex> l(mu);
        tracks.insert(pos.cast_to<list_type::iterator>(), first, last);
        l.unlock();
        if(size() == 1) {
            current_track_ = begin();
            trackChanged(*current_track_, true);
        }
    }

    void push_back(Playlist::value_type&& value) {
        unique_lock<Mutex> l(mu);
        tracks.push_back(std::move(value));
        l.unlock();
        if(size() == 1) {
            current_track_ = tracks.begin();
            trackChanged(*current_track_, currentTrack() != end());
        }
    }

    boost::signals2::connection connectTrackChangedSlot(const TrackChangedSignal::slot_type& slot) {
        return trackChanged.connect(slot);
    }

private:
    Logger::Logger logject;
    Playlist::iterator current_track_;
    typedef boost::container::stable_vector<Playlist::value_type> list_type;
    list_type tracks;
    Playlist::TrackChangedSignal trackChanged;
    typedef mutex Mutex;
    mutex mu;
    friend class Playlist;
};

Playlist::Playlist() : pimpl(new impl) {}

Playlist::~Playlist() {}

//IO
std::streamsize Playlist::read(char* s, std::streamsize n) {
    return pimpl->read(s, n);
}

void Playlist::seek(chrono::milliseconds dur) {
    pimpl->seek(dur);
}

//playlist controls
chrono::milliseconds Playlist::duration() const {
    return std::move(pimpl->duration());
}

void Playlist::previous() {
    pimpl->previous();
}

void Playlist::next() {
    pimpl->next();
}

Playlist::iterator& Playlist::currentTrack() {
    return pimpl->currentTrack();
}

Playlist::const_iterator Playlist::currentTrack() const {
    return pimpl->currentTrack();
}

Playlist::reference Playlist::operator[](size_type pos) {
    return pimpl->tracks[pos];
}

Playlist::const_reference Playlist::operator[](size_type pos) const {
    return pimpl->tracks[pos];
}

//element access
Playlist::reference Playlist::front() {
    return pimpl->front();
}

Playlist::const_reference Playlist::front() const {
    return pimpl->front();
}

Playlist::reference Playlist::back() {
    return pimpl->back();
}

Playlist::const_reference Playlist::back() const {
    return pimpl->back();
}

//iterators
Playlist::iterator Playlist::begin() {
    return pimpl->begin();
}

Playlist::const_iterator Playlist::begin() const {
    return pimpl->begin();
}

Playlist::iterator Playlist::end() {
    return pimpl->end();
}

Playlist::const_iterator Playlist::end() const {
    return pimpl->end();
}

//capacity
bool Playlist::empty() const{
    return pimpl->empty();
}

Playlist::size_type Playlist::size() const {
    return pimpl->size();
}

Playlist::size_type Playlist::max_size() const {
    return std::numeric_limits<size_type>::max();
}

Playlist::iterator Playlist::insert(Playlist::iterator pos, Playlist::value_type&& value) {
    return pimpl->insert(pos, std::move(value));
}

void Playlist::insert(iterator pos, iterator first, iterator last) {
    pimpl->insert(pos, first, last);
}

void Playlist::push_back(Playlist::value_type&& value) {
    pimpl->push_back(std::move(value));
}

boost::signals2::connection Playlist::connectTrackChangedSlot(const TrackChangedSignal::slot_type& slot) {
    return pimpl->connectTrackChangedSlot(slot);
}

void Playlist::swap(Playlist& b)  {
    std::swap(pimpl->tracks, b.pimpl->tracks);
}

}//end namespace Melosic
