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
#include <melosic/core/logging.hpp>

namespace Melosic {
class Kernel;
class Playlist;
namespace Output {
enum class DeviceState;
}
}
using namespace Melosic;
using Output::DeviceState;

class PlaylistModel;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<Kernel> kernel, QWidget * parent = 0);
    ~MainWindow();
    void onStateChangeSlot(DeviceState state);

private Q_SLOTS:
    void on_actionOpen_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionNext_triggered();
    void on_outputDevicesCBX_currentIndexChanged(int index);
    void on_actionOptions_triggered();

private:
    Ui::MainWindow* ui;
    std::shared_ptr<Kernel> kernel;
    std::shared_ptr<Playlist> currentPlaylist;
    PlaylistModel* playlistModel;
    Player& player;
    Logger::Logger logject;
    std::list<boost::signals2::scoped_connection> ssConnections;
    int oldDeviceIndex;
};

#endif // MELOSIC_MAINWINDOW_H
