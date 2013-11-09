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

#ifndef MELOSIC_MAINWINDOW_H
#define MELOSIC_MAINWINDOW_H

#include <QScopedPointer>

#include <list>

#include <melosic/melin/logging.hpp>
#include <melosic/common/connection.hpp>

class QQmlEngine;
class QQmlContext;
class QQmlComponent;
class QQuickWindow;

#ifdef Qt5Test_FOUND
class ModelTest;
#endif

namespace Melosic {

namespace Core {
class Kernel;
class Player;
}
class PlayerControls;

namespace Output {
enum class DeviceState;
}

class PlaylistManagerModel;

class MELOSIC_EXPORT MainWindow {
public:
    explicit MainWindow(Core::Kernel& kernel, Core::Player& player);
    ~MainWindow();
    void onStateChangeSlot(Output::DeviceState state);

private:
    Logger::Logger logject;
    std::list<Signals::ScopedConnection> scopedSigConns;
    QScopedPointer<PlayerControls> playerControls;

    QScopedPointer<QQmlEngine> engine;
    QScopedPointer<QQmlComponent> component;
    QScopedPointer<QQuickWindow> window;
    PlaylistManagerModel* playlistManagerModel;
#ifdef Qt5Test_FOUND
    ModelTest* modelTest;
#endif
};

} // namespace Melosic

#endif // MELOSIC_MAINWINDOW_H
