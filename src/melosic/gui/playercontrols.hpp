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
#include <QString>

#include <memory>
#include <chrono>
namespace chrono = std::chrono;

namespace Melosic {

namespace Core {
class Kernel;
class Player;
}

class PlaylistModel;

class PlayerControls : public QObject {
    Q_OBJECT
    struct impl;
    std::unique_ptr<impl> pimpl;

    Q_ENUMS(DeviceState)
    Q_PROPERTY(DeviceState state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString stateStr READ stateStr NOTIFY stateStrChanged)

public:
    explicit PlayerControls(Core::Kernel& kernel, Core::Player& player, QObject* parent = 0);

    enum DeviceState {
        Error,
        Ready,
        Playing,
        Paused,
        Stopped,
        Initial,
    };

    virtual ~PlayerControls();

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();
    Q_INVOKABLE void jumpTo(int);
    Q_INVOKABLE void seek(chrono::milliseconds);

    DeviceState state() const;
    QString stateStr() const;

Q_SIGNALS:
    void stateChanged(DeviceState);
    void stateStrChanged(QString);
};

} // namespace Melosic

#endif // MELOSIC_PLAYERCONTROLS_HPP
