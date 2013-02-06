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
#include <boost/signals2/connection.hpp>

#include <melosic/melin/logging.hpp>

namespace Melosic {
class Track;
class Playlist;
namespace Output {
enum class DeviceState;
}
}

namespace LastFM {

class Service;
struct Method;
struct Track;

class Scrobbler : public std::enable_shared_from_this<Scrobbler> {
public:
    Scrobbler(std::shared_ptr<Service> lastserv);

    std::shared_ptr<Track> currentTrack();
    void updateNowPlaying(const Track& track);
    void updateNowPlaying(std::shared_ptr<Method> track);
    void cacheTrack(const Melosic::Track& track);
    void cacheTrack(std::shared_ptr<Method> track);
    void cacheCurrentTrack();
    void submitCache();
    void notifySlot(chrono::milliseconds current, chrono::milliseconds total);
    void stateChangedSlot(Melosic::Output::DeviceState state);
    void playlistChangeSlot(std::shared_ptr<Melosic::Playlist> playlist);
    void trackChangedSlot(const Melosic::Track& newTrack, bool alive);

private:
    std::shared_ptr<Service> lastserv;
    Melosic::Logger::Logger logject;
    std::shared_ptr<Method> currentTrackData;
    std::shared_ptr<Track> currentTrack_;
    boost::signals2::scoped_connection playlistConn;
};

}

#endif // LASTFM_SCROBBLER_HPP
