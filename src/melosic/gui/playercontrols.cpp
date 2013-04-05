/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#include <melosic/core/player.hpp>

#include "playercontrols.hpp"

namespace Melosic {

class PlayerControls::PlayerControlsPrivate {
    Q_DECLARE_PUBLIC(PlayerControls)
    Core::Player& player;
    PlayerControls* const q_ptr;
public:
    PlayerControlsPrivate(Core::Player& player, PlayerControls* parent) : player(player), q_ptr(parent) {}
};

PlayerControls::PlayerControls(Core::Player& player, QObject *parent) :
    QObject(parent), d_ptr(new PlayerControlsPrivate(player, this))
{}

PlayerControls::~PlayerControls() {}

void PlayerControls::play() {
    d_func()->player.play();
}

void PlayerControls::pause() {
    d_func()->player.pause();
}

void PlayerControls::stop() {
    d_func()->player.stop();
}

void PlayerControls::previous() {
}

void PlayerControls::next() {
}

void PlayerControls::seek() {
}

} // namespace Melosic
