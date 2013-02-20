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

#include <list>

#include <QStandardPaths>
#include <QMessageBox>
#include <QFileDialog>

#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/sigslots/signals_fwd.hpp>
#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/melin/sigslots/slots.hpp>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "trackseeker.hpp"
#include "playlistmodel.hpp"
#include "configurationdialog.hpp"

MainWindow::MainWindow(Kernel& kernel, QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    kernel(kernel),
    currentPlaylist(std::move(std::make_shared<Playlist>(kernel.getSlotManager()))),
    playlistModel(new PlaylistModel(kernel, currentPlaylist)),
    player(kernel.getPlayer()),
    logject(logging::keywords::channel = "MainWindow")
{
    ui->setupUi(this);
    ui->stopButton->setDefaultAction(ui->actionStop);
    ui->playButton->setDefaultAction(ui->actionPlay);
    ui->nextButton->setDefaultAction(ui->actionNext);
    ui->playlistView->setModel(playlistModel);

    Slots::Manager& slotman = kernel.getSlotManager();

    scopedSigConns.emplace_back(slotman.get<Signals::Player::StateChanged>()
                               .connect(std::bind(&MainWindow::onStateChangeSlot, this, ph::_1)));
    scopedSigConns.emplace_back(ui->trackSeeker->get<Signals::TrackSeeker::Seek>()
                               .emplace_connect(&Player::seek, &player, ph::_1));
    ui->trackSeeker->connectSlots(&slotman);
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
                                                   QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
                                                   tr("Audio Files (*.flac)"), 0,
                                                   QFileDialog::ReadOnly);
    std::list<std::string> names;
    for(const auto& filename : filenames) {
        names.push_back(filename.toStdString());
    }
    QStringList fails = playlistModel->appendFiles(names);
    if(fails.size()) {
        QMessageBox failmsg(QMessageBox::Warning, "Failed to add tracks", "", QMessageBox::Ok, this);
        failmsg.setText(QString::number(fails.size()) + " tracks could not be added.");
        failmsg.setDetailedText(fails.join("\n"));
        failmsg.exec();
    }
}

void MainWindow::on_actionPlay_triggered() {
    if(currentPlaylist->size()) {
        if(!player.currentPlaylist()) {
            player.openPlaylist(currentPlaylist);
        }
        if(bool(player)) {
            if(player.state() == Output::DeviceState::Playing) {
                player.pause();
            }
            else if(player.state() != Output::DeviceState::Playing) {
                player.play();
            }
        }
    }
}

void MainWindow::on_actionStop_triggered() {
    if(bool(player)) {
        player.stop();
    }
}

void MainWindow::on_actionNext_triggered() {
    if(bool(player)) {
        currentPlaylist->next();
    }
}

void MainWindow::on_actionOptions_triggered() {
    ConfigurationDialog c(kernel.getConfigManager());
    c.exec();
}
