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

#include <numeric>
#include <thread>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
using boost::shared_mutex;
using Mutex = shared_mutex;
using lock_guard = std::lock_guard<Mutex>;
using unique_lock = std::unique_lock<Mutex>;
using shared_lock_guard = boost::shared_lock_guard<Mutex>;
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm_ext/insert.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/adaptor/sliced.hpp>
using namespace boost::adaptors;

#include <melosic/core/track.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/sigslots/signals_fwd.hpp>
#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include <melosic/melin/decoder.hpp>

#include "playlist.hpp"

namespace Melosic {
namespace Core {

class Playlist::impl {
public:
    impl(const std::string& name, Slots::Manager& slotman, Decoder::Manager& decman)
        : current_track_(begin()),
          name(name),
          trackChanged(slotman.get<Signals::Playlist::TrackChanged>()),
          decman(decman),
          logject(logging::keywords::channel = "Playlist")
    {}

    //IO
    std::streamsize read(char* s, std::streamsize n) {
        if(currentTrack() == end()) {
            return -1;
        }
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
            lock_guard l(mu);
            current_track_->reset();
            --current_track_;
        }
        trackChanged(*currentTrack());
    }

    void next() {
        if(currentTrack() != end()) {
            lock_guard l(mu);
            current_track_->reset();
            ++current_track_;
        }
        if(currentTrack() != end())
            trackChanged(*currentTrack());
    }

    void jumpTo(size_type pos) {
        {
            lock_guard l(mu);
            if(current_track_ != tracks.end())
                current_track_->reset();
            current_track_ = std::begin(tracks) + pos;
        }
        trackChanged(*currentTrack());
    }

    Playlist::iterator& currentTrack() {
        shared_lock_guard l(mu);
        return current_track_;
    }

    //element access
    Playlist::reference front() {
        shared_lock_guard l(mu);
        return tracks.front();
    }

    Playlist::reference back() {
        shared_lock_guard l(mu);
        return tracks.back();
    }

    Playlist::reference get(size_type pos) {
        shared_lock_guard l(mu);
        return tracks[pos];
    }

    //iterators
    Playlist::iterator begin() {
        shared_lock_guard l(mu);
        return std::begin(tracks);
    }

    Playlist::iterator end() {
        shared_lock_guard l(mu);
        return std::end(tracks);
    }

    //capacity
    bool empty() {
        shared_lock_guard l(mu);
        return tracks.empty();
    }

    Playlist::size_type size() {
        shared_lock_guard l(mu);
        return tracks.size();
    }

    Playlist::size_type max_size() const {
        return std::numeric_limits<Playlist::size_type>::max();
    }

    Playlist::iterator insert(Playlist::const_iterator pos, Playlist::value_type&& value) {
        mu.lock();
        auto r = tracks.insert(pos, value);
        mu.unlock();
        if(size() == 1) {
            mu.lock();
            current_track_ = r;
            mu.unlock();
            trackChanged(*currentTrack());
        }
        return r;
    }

    void insert(Playlist::const_iterator pos, Playlist::iterator first, Playlist::iterator last) {
        mu.lock();
        auto r = tracks.insert(pos, first, last);
        mu.unlock();
        if(size() == 1) {
            mu.lock();
            current_track_ = r;
            mu.unlock();
            trackChanged(*currentTrack());
        }
    }

    Playlist::iterator emplace(Playlist::const_iterator pos,
                               const boost::filesystem::path& filename,
                               chrono::milliseconds start,
                               chrono::milliseconds end)
    {
        unique_lock l(mu);

        auto r = tracks.emplace(pos, decman, filename, start, end);
        l.unlock();

        if(size() == 1) {
            l.lock();
            current_track_ = std::begin(tracks);
            l.unlock();
            trackChanged(*currentTrack());
        }

        return r;
    }

    Playlist::iterator emplace(Playlist::const_iterator pos, ForwardRange<const boost::filesystem::path> values) {
        unique_lock l(mu);
        if(values.empty())
            return std::next(std::begin(tracks), pos - std::begin(tracks));

        auto r = std::next(std::begin(tracks), pos - std::begin(tracks) + 1);
        for(auto& path : values) {
            pos = tracks.emplace(pos, decman, path);
        }
        l.unlock();
        if(size() == 1) {
            l.lock();
            current_track_ = std::begin(tracks);
            l.unlock();
            trackChanged(*currentTrack());
        }

        return r;
    }

    void push_back(Playlist::value_type&& value) {
        mu.lock();
        tracks.push_back(std::move(value));
        mu.unlock();
        if(size() == 1) {
            mu.lock();
            current_track_ = std::begin(tracks);
            mu.unlock();
            trackChanged(*currentTrack());
        }
    }

    void emplace_back(const boost::filesystem::path& filename,
                      chrono::milliseconds start,
                      chrono::milliseconds end)
    {
        unique_lock l(mu);
        tracks.emplace_back(decman, filename, start, end);
        l.unlock();
        if(size() == 1) {
            l.lock();
            current_track_ = std::begin(tracks);
            l.unlock();
            trackChanged(*currentTrack());
        }
    }

    Playlist::iterator erase(Playlist::const_iterator pos) {
        lock_guard l(mu);
        return tracks.erase(pos);
    }

    Playlist::iterator erase(Playlist::size_type start, Playlist::size_type end) {
        lock_guard l(mu);
        boost::range::erase(tracks, tracks | sliced(start, end));
        return std::next(tracks.begin(), start);
    }

    void clear() {
        lock_guard l(mu);
        tracks.clear();
    }

    const std::string& getName() {
        shared_lock_guard l(mu);
        return name;
    }

    void setName(const std::string& name) {
        lock_guard l(mu);
        this->name = name;
    }

private:
    Mutex mu;
    friend struct Block;
    Playlist::list_type tracks;
    Playlist::iterator current_track_;
    std::string name;
    Signals::Playlist::TrackChanged& trackChanged;
    Decoder::Manager& decman;
    Logger::Logger logject;
};

//Logger::Logger Playlist::impl::logject(logging::keywords::channel = "Playlist");

Playlist::Playlist(const std::string& name, Slots::Manager& slotman, Decoder::Manager& decman)
    : pimpl(new impl(name, slotman, decman)) {}

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

void Playlist::jumpTo(Playlist::size_type pos) {
    pimpl->jumpTo(pos);
}

Playlist::iterator& Playlist::currentTrack() {
    return pimpl->currentTrack();
}

Playlist::const_iterator Playlist::currentTrack() const {
    return pimpl->currentTrack();
}

Playlist::reference Playlist::operator[](size_type pos) {
    return pimpl->get(pos);
}

Playlist::const_reference Playlist::operator[](size_type pos) const {
    return pimpl->get(pos);
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

Playlist::iterator Playlist::insert(Playlist::const_iterator pos, Playlist::value_type&& value) {
    return pimpl->insert(pos, std::move(value));
}

Playlist::iterator Playlist::emplace(Playlist::const_iterator pos,
                                     const boost::filesystem::path& filename,
                                     chrono::milliseconds start,
                                     chrono::milliseconds end)
{
    return pimpl->emplace(pos, filename, start, end);
}

Playlist::iterator Playlist::emplace(Playlist::const_iterator pos, ForwardRange<const boost::filesystem::path> values) {
    return pimpl->emplace(pos, values);
}

void Playlist::push_back(Playlist::value_type&& value) {
    pimpl->push_back(std::move(value));
}

void Playlist::emplace_back(const boost::filesystem::path& filename,
                            chrono::milliseconds start,
                            chrono::milliseconds end)
{
    pimpl->emplace_back(filename, start, end);
}

Playlist::iterator Playlist::erase(const_iterator pos) {
    return pimpl->erase(pos);
}

Playlist::iterator Playlist::erase(Playlist::size_type start, Playlist::size_type end) {
    return pimpl->erase(start, end);
}

void Playlist::clear() {
    pimpl->clear();
}

void Playlist::swap(Playlist& b)  {
    std::swap(pimpl, b.pimpl);
}

const std::string& Playlist::getName() const {
    return pimpl->getName();
}

void Playlist::setName(const std::string& name) {
    pimpl->setName(name);
}

} // namespace Core
} // namespace Melosic
