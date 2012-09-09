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

#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/player.hpp>
using namespace Melosic;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(Kernel& kernel, QWidget * parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void on_actionOpen_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();

private:
    Ui::MainWindow * ui;
    Kernel& kernel;
    std::shared_ptr<IO::File> file;
    std::shared_ptr<Track> track;
    Player player;
};

#endif // MELOSIC_MAINWINDOW_H
