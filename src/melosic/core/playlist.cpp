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

#include <melosic/core/track.hpp>
#include "playlist.hpp"

namespace Melosic {

Playlist::Playlist() : current_track(begin()) {}

Playlist::~Playlist() {}

Playlist::Playlist(const Playlist& b) {

}

Playlist& Playlist::operator=(const Playlist& b) {
    return *this;
}

Playlist& Playlist::operator=(Playlist&& b) {
    return *this;
}

//IO
std::streamsize Playlist::read(char * s, std::streamsize n) {
    if(current() != end()) {
        auto r = current()->read(s, n);
        if(r < n) {
            current()->close();
            next();
            if(current() != end()) {
                current()->reOpen();
                r += current()->read(s + r, n - r);
            }
        }
        return r;
    }
    return -1;
}

void Playlist::seek(std::chrono::milliseconds dur) {
    if(current() != end()) {
        current()->seek(dur);
    }
}

//playlist controls
std::chrono::milliseconds Playlist::duration() {
    return std::accumulate(begin(), end(), std::chrono::milliseconds(0),
                           [](std::chrono::milliseconds a, Track b) { return a + b.duration();});
}

void Playlist::previous() {
    if(current() == begin()) {
        seek(std::chrono::milliseconds(0));
    }
    else if(size() >= 1) {
        std::lock_guard<Mutex> l(mu);
        current_track--;
    }
    trackChanged();
}

void Playlist::next() {
    if(current() != end()) {
        std::lock_guard<Mutex> l(mu);
        current_track++;
    }
    trackChanged();
}

Playlist::iterator& Playlist::current() {
    std::lock_guard<Mutex> l(mu);
    return current_track;
}

Playlist::const_iterator Playlist::current() const {
    return current_track;
}

Playlist::reference Playlist::operator[](size_type pos) {
    return getTrack(pos, begin(), std::iterator_traits<iterator>::iterator_category());
}

Playlist::const_reference Playlist::operator[](size_type pos) const {
    return getTrack(pos, begin(), std::iterator_traits<iterator>::iterator_category());
}

//element access
Playlist::reference Playlist::front() {
    return tracks.front();
}

Playlist::const_reference Playlist::front() const {
    return tracks.front();
}

Playlist::reference Playlist::back() {
    return tracks.back();
}

Playlist::const_reference Playlist::back() const {
    return tracks.back();
}

//iterators
Playlist::iterator Playlist::begin() {
    return tracks.begin();
}

Playlist::const_iterator Playlist::begin() const {
    return tracks.begin();
}

Playlist::iterator Playlist::end() {
    return tracks.end();
}

Playlist::const_iterator Playlist::end() const {
    return tracks.end();
}

//capacity
bool Playlist::empty() const{
    return tracks.empty();
}

Playlist::size_type Playlist::size() const {
    return tracks.size();
}

Playlist::size_type Playlist::max_size() const {
    return std::numeric_limits<size_type>::max();
}

//modifiers
Playlist::iterator Playlist::insert(Playlist::iterator pos, const Playlist::value_type& value) {
    auto r = tracks.insert(pos, value);
    if(size() == 1) {
        current_track = r;
    }
    return r;
}

Playlist::iterator Playlist::insert(Playlist::iterator pos, Playlist::value_type&& value) {
    auto r = tracks.insert(pos, value);
    if(size() == 1) {
        current_track = r;
    }
    return r;
}

void Playlist::push_back(const Playlist::value_type& value) {
    tracks.push_back(value);
    if(size() == 1) {
        current_track = tracks.begin();
    }
}

void Playlist::push_back(Playlist::value_type&& value) {
    tracks.push_back(value);
    if(size() == 1) {
        current_track = tracks.begin();
    }
}

}//end namespace Melosic
