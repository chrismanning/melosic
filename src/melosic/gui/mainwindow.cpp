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
#include <QMessageBox>
#include <list>

#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>

MainWindow::MainWindow(QWidget * parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    currentPlaylist(new Playlist),
    playlistModel(new PlaylistModel(currentPlaylist)),
    player(new Player),
    logject(boost::log::keywords::channel = "MainWindow")
{
    ui->setupUi(this);
    ui->stopButton->setDefaultAction(ui->actionStop);
    ui->playButton->setDefaultAction(ui->actionPlay);
    ui->nextButton->setDefaultAction(ui->actionNext);
    ui->playlistView->setModel(playlistModel);

    ssConnections.emplace_back(player->connectState(boost::bind(&MainWindow::onStateChangeSlot, this, _1)));
    ssConnections.emplace_back(ui->trackSeeker->connectSeek(
                                   TrackSeeker::SeekSignal::slot_type(&Player::seek, player.get(), _1)
                                   .track_foreign(std::weak_ptr<Player>(player))));
    ssConnections.emplace_back(player->connectState(boost::bind(&TrackSeeker::onStateChangeSlot,
                                                                ui->trackSeeker, _1)));
    ssConnections.emplace_back(player->connectNotifySlot(boost::bind(&TrackSeeker::onNotifySlot,
                                                                     ui->trackSeeker, _1, _2)));

    auto devs = Kernel::getInstance().getOutputDeviceNames();
    for(const auto& dev : devs) {
        ui->outputDevicesCBX->addItem(QString::fromStdString(dev.getDesc()),
                                      QString::fromStdString(dev.getName()));
    }
    oldDeviceIndex = ui->outputDevicesCBX->currentIndex();
}

MainWindow::~MainWindow() {
    TRACE_LOG(logject) << "Destroying main window";
    delete ui;
    delete playlistModel;
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
    std::list<std::string> names;
    for(const auto& filename : filenames) {
        names.push_back(filename.toStdString());
    }
    playlistModel->appendFiles(names);
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
    }
}

void MainWindow::on_actionStop_triggered() {
    if(bool(player) && bool(*player)) {
        player->stop();
    }
}

void MainWindow::on_actionNext_triggered() {
    if(bool(player) && bool(*player)) {
        currentPlaylist->next();
    }
}

void MainWindow::on_outputDevicesCBX_currentIndexChanged(int index) {
    try {
        LOG(logject) << "Changing output device to: " << ui->outputDevicesCBX->itemText(index).toStdString();

        std::chrono::milliseconds time(0);
        auto state = player->state();
        if(state != Output::DeviceState::Stopped) {
            time = player->tell();
            player->stop();
        }

        player->changeOutput(Kernel::getInstance()
                             .getOutputDevice(ui->outputDevicesCBX->itemData(index).value<QString>().toStdString()));

        switch(state) {
            case DeviceState::Playing:
                player->play();
                player->seek(time);
                break;
            case DeviceState::Paused:
                player->play();
                player->seek(time);
                player->pause();
                break;
            case DeviceState::Error:
            case DeviceState::Stopped:
            case DeviceState::Ready:
            default:
                break;
        }
        oldDeviceIndex = index;
    }
    catch(std::exception& e) {
        LOG(logject) << "Exception caught: " << e.what();

        player->stop();

        QMessageBox error;
        error.setText("EXception");
        error.setDetailedText(e.what());
        error.exec();
        ui->outputDevicesCBX->setCurrentIndex(oldDeviceIndex);
    }
}
