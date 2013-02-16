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

#include <list>

#include <melosic/core/track.hpp>

#include "signals.hpp"
#include "slots.hpp"

namespace Melosic {
namespace Slots {

class Manager::impl {
    Signals::Player::StateChanged stateChanged;
    Signals::Player::NotifyPlayPos notifyPos;
    Signals::Player::PlaylistChanged playlistChanged;
    Signals::Playlist::TrackChanged trackChanged;
    Signals::TrackSeeker::Seek seek;
    Signals::Output::PlayerSinkChanged playerSinkChanged;
    Signals::Output::ReqSinkChange requestSinkChange;
    Signals::Config::Loaded loaded;

    friend class Manager;
};

Manager::Manager() : pimpl(new impl) {}

Manager::~Manager() {}

template <>
Signals::Player::StateChanged& Manager::get<Signals::Player::StateChanged>() {
    return pimpl->stateChanged;
}
template Signals::Player::StateChanged& Manager::get<Signals::Player::StateChanged>();

template<>
Signals::Player::NotifyPlayPos& Manager::get<Signals::Player::NotifyPlayPos>() {
    return pimpl->notifyPos;
}
template Signals::Player::NotifyPlayPos& Manager::get<Signals::Player::NotifyPlayPos>();

template<>
Signals::Player::PlaylistChanged& Manager::get<Signals::Player::PlaylistChanged>() {
    return pimpl->playlistChanged;
}
template Signals::Player::PlaylistChanged& Manager::get<Signals::Player::PlaylistChanged>();

template <>
Signals::Playlist::TrackChanged& Manager::get<Signals::Playlist::TrackChanged>() {
    return pimpl->trackChanged;
}
template Signals::Playlist::TrackChanged& Manager::get<Signals::Playlist::TrackChanged>();

template <>
Signals::TrackSeeker::Seek& Manager::get<Signals::TrackSeeker::Seek>() {
    return pimpl->seek;
}
template Signals::TrackSeeker::Seek& Manager::get<Signals::TrackSeeker::Seek>();

template <>
Signals::Output::PlayerSinkChanged& Manager::get<Signals::Output::PlayerSinkChanged>() {
    return pimpl->playerSinkChanged;
}
template Signals::Output::PlayerSinkChanged& Manager::get<Signals::Output::PlayerSinkChanged>();

template <>
Signals::Output::ReqSinkChange& Manager::get<Signals::Output::ReqSinkChange>() {
    return pimpl->requestSinkChange;
}
template Signals::Output::ReqSinkChange& Manager::get<Signals::Output::ReqSinkChange>();

template <>
Signals::Config::Loaded& Manager::get<Signals::Config::Loaded>() {
    return pimpl->loaded;
}
template Signals::Config::Loaded& Manager::get<Signals::Config::Loaded>();

} // namespace Slots
} // namespace Melosic