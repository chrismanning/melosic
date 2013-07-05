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

#include <melosic/melin/sigslots/signals_fwd.hpp>
#include <melosic/common/common.hpp>

namespace Melosic {
namespace Output {
class PlayerSink;
enum class DeviceState;
class Manager;
}

namespace Slots {
class Manager;
}

namespace Playlist {
class Manager;
}

using Output::DeviceState;

namespace Core {

class Playlist;

class MELOSIC_CORE_EXPORT Player {
public:
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(Player&&) = delete;

    void play();
    void pause();
    void stop();
    DeviceState state() const;
    void seek(chrono::milliseconds dur);
    chrono::milliseconds tell() const;

    Signals::Player::StateChanged& stateChangedSignal() const;

private:
    Player(Melosic::Playlist::Manager&, Output::Manager&, Slots::Manager&);

    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_PLAYER_HPP
