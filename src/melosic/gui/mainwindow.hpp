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

#include <QDebug>
#include <QMainWindow>
#include <QApplication>
#include <QList>
#include <memory>
#include <boost/signals2.hpp>

#include <melosic/core/player.hpp>
#include <melosic/common/logging.hpp>

namespace Melosic {
class Kernel;
class Playlist;
namespace Output {
enum class DeviceState;
}
}
using namespace Melosic;
using Output::DeviceState;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Kernel& kernel, QWidget * parent = 0);
    ~MainWindow();
    void onStateChangeSlot(DeviceState state);

private Q_SLOTS:
    void on_actionOpen_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();

    void on_outputDevicesCBX_currentIndexChanged(int index);

private:
    Ui::MainWindow * ui;
    Kernel& kernel;
    boost::shared_ptr<Playlist> currentPlaylist;
    boost::shared_ptr<Player> player;
    Logger::Logger logject;
    boost::signals2::connection playerStateConnection;
    boost::signals2::connection seekerStateConnection;
    boost::signals2::connection seekerNotifyConnection;
    boost::signals2::connection playerSeekConnection;
    int oldDeviceIndex;
};

#endif // MELOSIC_MAINWINDOW_H
