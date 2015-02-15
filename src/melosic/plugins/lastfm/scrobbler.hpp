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

#ifndef LASTFM_SCROBBLER_HPP
#define LASTFM_SCROBBLER_HPP

#include <memory>
#include <chrono>
namespace chrono = std::chrono;
#include <list>
#include <thread>
using std::mutex; using std::lock_guard; using std::unique_lock;

#include <melosic/melin/logging.hpp>
#include <melosic/common/connection.hpp>
#include <melosic/melin/output.hpp>

#include "track.hpp"

namespace Melosic {

namespace Core {
class Track;
class Playlist;
}

}

namespace lastfm {

class service;
struct Method;

class scrobbler : public std::enable_shared_from_this<scrobbler> {
public:
    explicit scrobbler(std::shared_ptr<service> lastserv);

    std::shared_ptr<track> currentTrack();
    void updateNowPlaying(std::shared_ptr<track> track);
    void cacheTrack(std::shared_ptr<track> track);
    void submitCache();
    void notifySlot(chrono::milliseconds current, chrono::milliseconds total);
    void stateChangedSlot(Melosic::Output::DeviceState);
    void playlistChangeSlot(std::shared_ptr<Melosic::Core::Playlist> playlist);
    void trackChangedSlot(const Melosic::Core::Track& newTrack);

private:
    std::shared_ptr<service> lastserv;
    Melosic::Logger::Logger logject;
    std::shared_ptr<track> currentTrack_;
    std::list<std::shared_ptr<track>> cache;
    Melosic::Signals::ScopedConnection playlistConn;
    std::list<Melosic::Signals::ScopedConnection> connections;
    Melosic::Output::DeviceState state = DeviceState::Stopped;

    typedef mutex Mutex;
    Mutex mu;
};

}

#endif // LASTFM_SCROBBLER_HPP
