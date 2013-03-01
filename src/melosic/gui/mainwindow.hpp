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

#include <memory>

#include <QDebug>
#include <QMainWindow>
#include <QApplication>
#include <QList>

#include <melosic/melin/logging.hpp>
#include <melosic/melin/sigslots/connection.hpp>

namespace Melosic {
class Kernel;
class Playlist;
class Player;
namespace Output {
enum class DeviceState;
}
}
using namespace Melosic;
using Output::DeviceState;

class PlaylistModel;
class KCategorizedView;
class QListView;
class QHBoxLayout;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Kernel& kernel, QWidget* parent = 0);
    ~MainWindow();
    void onStateChangeSlot(DeviceState state);
    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();

private Q_SLOTS:
    void on_actionOpen_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionNext_triggered();
    void on_actionOptions_triggered();

private:
    Kernel& kernel;
    std::shared_ptr<Playlist> currentPlaylist;
    PlaylistModel* playlistModel = nullptr;
    Player& player;
    Logger::Logger logject;
    std::list<Signals::ScopedConnection> scopedSigConns;
    QAction* actionOpen;
    QAction* actionPlay;
    QAction* actionExit;
    QAction* actionStop;
    QAction* actionPrevious;
    QAction* actionNext;
    QAction* actionOptions;
    QMenuBar* menubar;
    QMenu* menuFile;
    QMenu* menuPlayback;
    QMenu* menuTools;
    QStatusBar* statusbar;

    KCategorizedView* playlistView;
};

#endif // MELOSIC_MAINWINDOW_H
