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

#include <melosic/common/signal_fwd.hpp>
#include <melosic/core/player_signals.hpp>
#include <melosic/common/common.hpp>

namespace Melosic {
namespace Output {
enum class DeviceState;
class Manager;
}

namespace Playlist {
class Manager;
}

using Output::DeviceState;

namespace Core {

class Playlist;

class Player {
public:
    Player(Melosic::Playlist::Manager&, Output::Manager&);

    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(Player&&) = delete;

    MELOSIC_EXPORT void play();
    MELOSIC_EXPORT void pause();
    MELOSIC_EXPORT void stop();
    MELOSIC_EXPORT DeviceState state() const;
    MELOSIC_EXPORT void seek(chrono::milliseconds dur);
    MELOSIC_EXPORT chrono::milliseconds tell() const;

    MELOSIC_EXPORT Signals::Player::StateChanged& stateChangedSignal() const;

    struct impl;
private:
    std::unique_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_PLAYER_HPP
