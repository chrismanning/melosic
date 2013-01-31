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

#ifndef MELOSIC_PLAYER_HPP
#define MELOSIC_PLAYER_HPP

#include <memory>
#include <chrono>
namespace chrono = std::chrono;
#include <boost/signals2.hpp>

namespace Melosic {

namespace Output {
class DeviceSink;
enum class DeviceState;
}

using Output::DeviceState;

class Playlist;

class Player
{
public:
    Player();
    Player(std::unique_ptr<Output::DeviceSink> device);
    ~Player();

    Player(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(const Player&) = delete;

    void play();
    void pause();
    void stop();
    DeviceState state();
    void seek(chrono::milliseconds dur);
    chrono::milliseconds tell();
    void finish();
    void changeOutput(std::unique_ptr<Output::DeviceSink> device);
    explicit operator bool();
    void openPlaylist(std::shared_ptr<Playlist> playlist);
    std::shared_ptr<Playlist> currentPlaylist();

    typedef boost::signals2::signal<void(DeviceState)> StateSignal;
    boost::signals2::connection connectStateSlot(const StateSignal::slot_type& slot);

    typedef boost::signals2::signal<void(chrono::milliseconds, chrono::milliseconds)> NotifySignal;
    boost::signals2::connection connectNotifySlot(const NotifySignal::slot_type& slot);

    typedef boost::signals2::signal<void(std::shared_ptr<Playlist>)> PlaylistChangeSignal;
    boost::signals2::connection connectPlaylistChangeSlot(const PlaylistChangeSignal::slot_type& slot);
private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}

#endif // MELOSIC_PLAYER_HPP
