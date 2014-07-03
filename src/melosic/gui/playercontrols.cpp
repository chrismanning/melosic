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

#include <QMetaEnum>

#include <cassert>
#include <melosic/common/optional.hpp>

#include <melosic/melin/kernel.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/common/connection.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/playlist.hpp>

#include "playercontrols.hpp"
#include "playlistmodel.hpp"

namespace Melosic {

struct PlayerControls::impl {
    Core::Player& player;
    Signals::ScopedConnection stateChangedConn;

    impl(Core::Player& player, std::shared_ptr<Playlist::Manager> playman)
        : player(player)
    {
        state = (PlayerControls::DeviceState) player.state();
        playman->getCurrentPlaylistChangedSignal().
                connect([this] (optional<Core::Playlist> cp) {
            currentPlaylist = cp;
        });
        currentPlaylist = playman->currentPlaylist();
    }

    PlayerControls::DeviceState state;
    PlaylistModel* currentPlaylistModel{nullptr};
    optional<Core::Playlist> currentPlaylist;
};

PlayerControls::PlayerControls(Core::Player& player, const std::shared_ptr<Playlist::Manager>& playman, QObject* parent) :
    QObject(parent), pimpl(new impl(player, playman))
{
    qRegisterMetaType<DeviceState>("DeviceState");
    pimpl->stateChangedConn = pimpl->player.stateChangedSignal().connect([this] (Output::DeviceState ds) {
        pimpl->state = (PlayerControls::DeviceState) ds;
        Q_EMIT stateChanged(state());
        Q_EMIT stateStrChanged(stateStr());
    });
}

PlayerControls::~PlayerControls() {}

void PlayerControls::play() {
    pimpl->player.play();
}

void PlayerControls::pause() {
    pimpl->player.pause();
}

void PlayerControls::stop() {
    pimpl->player.stop();
}

void PlayerControls::previous() {
    pimpl->player.previous();
}

void PlayerControls::next() {
    pimpl->player.next();
}

void PlayerControls::jumpTo(int p) {
    pimpl->player.jumpTo(p);
}

void PlayerControls::seek(chrono::milliseconds t) {
    if(!pimpl->currentPlaylist)
        return;
    pimpl->player.seek(t);
}

PlayerControls::DeviceState PlayerControls::state() const {
    return pimpl->state;
}

QString PlayerControls::stateStr() const {
    for(int i=0; i < metaObject()->enumeratorCount(); ++i) {
        QMetaEnum m = metaObject()->enumerator(i);
        if(m.name() == QLatin1String("DeviceState"))
            return QLatin1String(m.valueToKey(state()));
    }
    assert(false);
}

} // namespace Melosic
