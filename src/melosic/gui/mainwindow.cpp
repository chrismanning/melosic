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

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;

#include <melosic/core/wav.hpp>

MainWindow::MainWindow(Kernel& kernel, QWidget * parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    kernel(kernel)/*,
    player(kernel.getOutputManager().getOutputDevice("front:CARD=PCH,DEV=0"))*/
{
    ui->setupUi(this);
    ui->stopButton->setDefaultAction(ui->actionStop);
    ui->playButton->setDefaultAction(ui->actionPlay);
}

MainWindow::~MainWindow()
{
    qDebug("Destroying main window");
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    auto filename = QFileDialog::getOpenFileName(this, tr("Open file"), ".", tr("Audio Files (*.flac)"));
    if(filename.length()) {
        file.reset(new IO::File(filename.toStdString()));

        track.reset(new Track(*file, kernel.getInputManager().getFactory(*file), std::chrono::milliseconds(0)));
        player.openStream(track);
    }
}

void MainWindow::on_actionPlay_triggered()
{
    if(bool(player)) {
        if(player.state() == Output::DeviceState::Playing) {
            player.pause();
            ui->playButton->setText("Play");
        }
        else if(player.state() != Output::DeviceState::Playing) {
            player.play();
            ui->playButton->setText("Pause");
        }
    }
    else {
        player.changeOutput(kernel.getOutputManager().getOutputDevice("front:CARD=PCH,DEV=0"));
    }
}

void MainWindow::on_actionStop_triggered()
{
    if(bool(player)) {
        player.stop();
        ui->playButton->setText("Play");
    }
}
