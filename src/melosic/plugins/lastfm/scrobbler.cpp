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

#include <melosic/core/track.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/output.hpp>
using Melosic::Output::DeviceState;
#include <melosic/melin/logging.hpp>
#include <melosic/common/signal.hpp>

#include "scrobbler.hpp"
#include "service.hpp"

namespace lastfm {

scrobbler::scrobbler(std::shared_ptr<service> lastserv) :
    lastserv(lastserv),
    logject(logging::keywords::channel = "lastfm::scrobbler")
{
    TRACE_LOG(logject) << "Scrobbling enabled";
//    connections.emplace_back(slotman->get<Melosic::Signals::Player::NotifyPlayPos>()
//            .connect(&Scrobbler::notifySlot, this, ph::_1, ph::_2));
    TRACE_LOG(logject) << "Connected notify slot";
//    connections.emplace_back(slotman->get<Melosic::Signals::Signal<void(Output::DeviceState)>>()
//            .connect(&Scrobbler::stateChangedSlot, this, ph::_1));
//    TRACE_LOG(logject) << "Connected state changed slot";
//    connections.emplace_back(slotman->get<Melosic::Signals::Playlist::PlaylistChanged>()
//            .connect(&Scrobbler::playlistChangeSlot, this, ph::_1));
    TRACE_LOG(logject) << "Connected playlist changed slot";
//    connections.emplace_back(slotman->get<Melosic::Signals::Playlist::TrackChanged>()
//            .connect(&Scrobbler::trackChangedSlot, this, ph::_1));
    TRACE_LOG(logject) << "Connected track changed slot";
}

void scrobbler::notifySlot(chrono::milliseconds current, chrono::milliseconds total) {
    static std::weak_ptr<track> prev;
    std::unique_lock<Mutex> l(mu);
    if(total > chrono::seconds(15) && current > (total / 2)
            && static_cast<bool>(currentTrack_)
            && currentTrack_ != prev.lock())
    {
        l.unlock();
        cacheTrack(currentTrack_);
        l.lock();
        prev = currentTrack_;
    }
}

void scrobbler::stateChangedSlot(DeviceState newstate) {
    unique_lock<Mutex> l(mu);
    state = newstate;
    if(newstate == DeviceState::Stopped) {
        l.unlock();
        submitCache();
        l.lock();
        currentTrack_.reset();
    }
    else if(newstate == DeviceState::Playing && static_cast<bool>(currentTrack_)) {
        l.unlock();
        updateNowPlaying(currentTrack_);
    }
}

void scrobbler::playlistChangeSlot(std::shared_ptr<Melosic::Core::Playlist> playlist) {
//    if(playlist && *playlist)
//        trackChangedSlot(*playlist->currentTrack());
}

void scrobbler::trackChangedSlot(const Melosic::Core::Track& newTrack) {
    TRACE_LOG(logject) << "Track changed";
    submitCache();
    std::unique_lock<Mutex> l(mu);
    currentTrack_ = std::make_shared<track>(lastserv, newTrack);
    if(state == DeviceState::Playing) {
        l.unlock();
        updateNowPlaying(currentTrack_);
    }
}

void scrobbler::submitCache() {
    TRACE_LOG(logject) << "Submitting cache";
    std::unique_lock<Mutex> l(mu);
    auto i = cache.begin();
    while(i != cache.end()) {
        auto ptr = *i;
        i = cache.erase(i);
        l.unlock();
        auto f = std::move(ptr->scrobble());
        l.lock();
        if(f.get()) {
        }
        else {
            i = ++cache.insert(i, ptr);
        }
    }
}

void scrobbler::cacheTrack(std::shared_ptr<track> track) {
    TRACE_LOG(logject) << "Caching track for submission";
    std::lock_guard<Mutex> l(mu);
    cache.push_back(track);
}

void scrobbler::updateNowPlaying(std::shared_ptr<track> track) {
    if(track)
        track->updateNowPlaying();
}

}
