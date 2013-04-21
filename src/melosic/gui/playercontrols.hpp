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

#ifndef MELOSIC_PLAYERCONTROLS_HPP
#define MELOSIC_PLAYERCONTROLS_HPP

#include <QObject>

namespace Melosic {

namespace Core {
class Player;
}

class PlayerControls : public QObject {
    Q_OBJECT
    class PlayerControlsPrivate;
    Q_DECLARE_PRIVATE(PlayerControls)
    const QScopedPointer<PlayerControlsPrivate> d_ptr;

public:
    explicit PlayerControls(Core::Player& player, QObject* parent = 0);

    virtual ~PlayerControls();

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();
    Q_INVOKABLE void seek();
};

} // namespace Melosic

#endif // MELOSIC_PLAYERCONTROLS_HPP