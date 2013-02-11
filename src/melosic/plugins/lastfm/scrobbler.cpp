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
#include <melosic/melin/sigslots/signals.hpp>

#include "scrobbler.hpp"
#include "service.hpp"
#include "track.hpp"

namespace LastFM {

Scrobbler::Scrobbler(std::shared_ptr<Service> lastserv) :
    lastserv(lastserv),
    logject(boost::log::keywords::channel = "LastFM::Scrobbler")
{}

void Scrobbler::notifySlot(chrono::milliseconds current, chrono::milliseconds total) {
    static std::weak_ptr<Method> prev;
    if(total > chrono::seconds(15) && current > (total / 2) && bool(currentTrackData) && currentTrackData != prev.lock()) {
        TRACE_LOG(logject) << "Caching track for submission";
        cacheTrack(currentTrackData);
        prev = currentTrackData;
    }
}

void Scrobbler::stateChangedSlot(DeviceState state) {
    if(state == DeviceState::Stopped) {
        TRACE_LOG(logject) << "Submitting cache";
        submitCache();
        currentTrackData.reset();
    }
    else if(state == DeviceState::Playing && bool(currentTrackData)) {
        updateNowPlaying(currentTrackData);
    }
}

void Scrobbler::playlistChangeSlot(std::shared_ptr<Melosic::Playlist> playlist) {
//    playlistConn = playlist->get<Melosic::Playlist::Signals::TrackChanged>()
//            .emplace_connect(&Scrobbler::trackChangedSlot,
//                             shared_from_this(),
//                             ph::_1, ph::_2);
    if(playlist->currentTrack() != playlist->end())
        trackChangedSlot(*playlist->currentTrack(), true);
}

void fun(Track& t) {
    std::clog << "Track URL: " << t.getUrl() << std::endl;
}

void Scrobbler::trackChangedSlot(const Melosic::Track& newTrack, bool alive) {
    if(!alive || !lastserv)
        return;
    currentTrack_ = std::make_shared<Track>(lastserv, newTrack);
//    currentTrack_->getInfo();
}

void Scrobbler::submitCache() {
}

void Scrobbler::cacheTrack(std::shared_ptr<Method> track) {
}

void Scrobbler::updateNowPlaying(std::shared_ptr<Method> track) {
}

}
