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
#include "trackseeker.hpp"
#include <QFileDialog>
#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;
#include <QDesktopServices>

#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>

MainWindow::MainWindow(Kernel& kernel, QWidget * parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    kernel(kernel),
    currentPlaylist(new Playlist),
    player(new Player),
    logject(boost::log::keywords::channel = "MainWindow")
{
    ui->setupUi(this);
    ui->stopButton->setDefaultAction(ui->actionStop);
    ui->playButton->setDefaultAction(ui->actionPlay);
    playerStateConnection = player->connectState(boost::bind(&MainWindow::onStateChangeSlot, this, _1));
    playerSeekConnection = ui->trackSeeker->connectSeek(boost::bind(&Player::seek, player.get(), _1));
    seekerStateConnection = player->connectState(boost::bind(&TrackSeeker::onStateChangeSlot, ui->trackSeeker, _1));
    seekerNotifyConnection = player->connectNotifySlot(boost::bind(&TrackSeeker::onNotifySlot, ui->trackSeeker, _1, _2));
}

MainWindow::~MainWindow() {
    TRACE_LOG(logject) << "Destroying main window";
    playerStateConnection.disconnect();
    delete ui;
}

void MainWindow::onStateChangeSlot(DeviceState state) {
    switch(state) {
        case DeviceState::Playing:
            ui->playButton->setText("Pause");
            break;
        case DeviceState::Error:
            ui->playButton->setText("Play");
            break;
        case DeviceState::Stopped:
            ui->playButton->setText("Play");
            break;
        case DeviceState::Paused:
            ui->playButton->setText("Resume");
            break;
        case DeviceState::Ready:
            ui->playButton->setText("Play");
        default:
            break;
    }
}

void MainWindow::on_actionOpen_triggered() {
    auto filenames = QFileDialog::getOpenFileNames(this, tr("Open file"),
                                                   QDesktopServices::storageLocation(QDesktopServices::MusicLocation),
                                                   tr("Audio Files (*.flac)"), 0,
                                                   QFileDialog::ReadOnly);
    for(const auto& filename : filenames) {
        std::unique_ptr<IO::File> file(new IO::File(filename.toStdString()));
        currentPlaylist->emplace_back(std::move(file),
                                      kernel.getInputManager().getFactory(*file),
                                      std::chrono::milliseconds(0));
    }
}

void MainWindow::on_actionPlay_triggered() {
    if(currentPlaylist->size()) {
        if(!player->currentPlaylist()) {
            player->openPlaylist(currentPlaylist);
        }
        if(bool(player) && bool(*player)) {
            if(player->state() == Output::DeviceState::Playing) {
                player->pause();
            }
            else if(player->state() != Output::DeviceState::Playing) {
                player->play();
            }
        }
        else {
            player->changeOutput(kernel.getOutputManager().getOutputDevice("front:CARD=PCH,DEV=0"));
            player->play();
        }
    }
}

void MainWindow::on_actionStop_triggered() {
    if(bool(player) && bool(*player)) {
        player->stop();
    }
}
