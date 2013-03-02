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
#include <QAction>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QListView>
#include <QScrollBar>

#include <kcategorizedview.h>
#include <kcategorizedsortfilterproxymodel.h>

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
#include "trackseeker.hpp"
#include "playlistmodel.hpp"
#include "playlistcategorymodel.hpp"
#include "configurationdialog.hpp"

MainWindow::MainWindow(Kernel& kernel, QWidget* parent) :
    QMainWindow(parent),
    kernel(kernel),
    currentPlaylist(std::move(std::make_shared<Playlist>(kernel.getSlotManager()))),
    playlistModel(new PlaylistModel(kernel, currentPlaylist)),
    player(kernel.getPlayer()),
    logject(logging::keywords::channel = "MainWindow")
{
    Slots::Manager& slotman = kernel.getSlotManager();

    scopedSigConns.emplace_back(slotman.get<Signals::Player::StateChanged>()
                               .connect(std::bind(&MainWindow::onStateChangeSlot, this, ph::_1)));
//    scopedSigConns.emplace_back(ui->trackSeeker->get<Signals::TrackSeeker::Seek>()
//                               .emplace_connect(&Player::seek, &player, ph::_1));
//    ui->trackSeeker->connectSlots(&slotman);

    actionOpen = new QAction("Open", this);
    actionOpen->setShortcut({"ctrl+o"});
    connect(actionOpen, &QAction::triggered, this, &MainWindow::on_actionOpen_triggered);
    actionPlay = new QAction("Play", this);
    actionPlay->setShortcut({"ctrl+p"});
    connect(actionPlay, &QAction::triggered, this, &MainWindow::on_actionPlay_triggered);
    actionExit = new QAction("Quit", this);
    actionExit->setShortcut({"ctrl+q"});
    connect(actionExit, &QAction::triggered, this, &MainWindow::close);
    actionStop = new QAction("Stop", this);
    connect(actionStop, &QAction::triggered, this, &MainWindow::on_actionStop_triggered);
    actionPrevious = new QAction("Previous", this);
    actionPrevious->setShortcut({"ctrl+b"});
    actionNext = new QAction("Next", this);
    connect(actionNext, &QAction::triggered, this, &MainWindow::on_actionNext_triggered);
    actionNext->setShortcut({"ctrl+n"});
    actionOptions = new QAction("Options", this);
    connect(actionOptions, &QAction::triggered, this, &MainWindow::on_actionOptions_triggered);
    menubar = new QMenuBar;
    menuFile = new QMenu("File", menubar);
    menuPlayback = new QMenu("Playback", menubar);
    menuTools = new QMenu("Tools", menubar);
    setMenuBar(menubar);
    statusbar = new QStatusBar;
    setStatusBar(statusbar);

    setMinimumSize(600, 400);

    playlistView = new KCategorizedView;
    auto categoryModel = new PlaylistCategoryModel;
    categoryModel->setSourceModel(playlistModel);
    categoryModel->setCategorizedModel(true);
    playlistView->setModel(categoryModel);
    playlistView->setCategoryDrawer(new CategoryDrawer(playlistView));
    playlistView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    playlistView->setCollapsibleBlocks(true);
    playlistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setCentralWidget(playlistView);

    connect(playlistView, &KCategorizedView::doubleClicked, [this](const QModelIndex& i) {
        if(i.isValid()) {
            currentPlaylist->jumpTo(i.row());
            play();
        }
    });

    menubar->addMenu(menuFile);
    menubar->addMenu(menuPlayback);
    menubar->addMenu(menuTools);
    menuFile->addAction(actionOpen);
    menuFile->addAction(actionExit);
    menuPlayback->addAction(actionPlay);
    menuPlayback->addAction(actionStop);
    menuPlayback->addAction(actionPrevious);
    menuPlayback->addAction(actionNext);
    menuTools->addAction(actionOptions);
}

MainWindow::~MainWindow() {
    TRACE_LOG(logject) << "Destroying main window";
    delete playlistModel;
}

void MainWindow::onStateChangeSlot(DeviceState /*state*/) {
//    switch(state) {
//        case DeviceState::Playing:
//            ui->playButton->setText("Pause");
//            break;
//        case DeviceState::Error:
//        case DeviceState::Stopped:
//        case DeviceState::Ready:
//            ui->playButton->setText("Play");
//            break;
//        case DeviceState::Paused:
//            ui->playButton->setText("Resume");
//            break;
//        default:
//            break;
//    }
}

void MainWindow::play() {
    actionPlay->trigger();
}

void MainWindow::stop() {
    actionStop->trigger();
}

void MainWindow::previous() {
    actionPrevious->trigger();
}

void MainWindow::next() {
    actionNext->trigger();
}

void MainWindow::on_actionOpen_triggered() {
    auto filenames = QFileDialog::getOpenFileNames(this, tr("Open file"),
                                                   QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
                                                   tr("Audio Files (*.flac)"), 0,
                                                   QFileDialog::ReadOnly);
    std::list<boost::filesystem::path> names;
    for(const auto& filename : filenames)
        names.emplace_back(filename.toStdString());

    QStringList fails = playlistModel->appendFiles(names);
    if(fails.size()) {
        QMessageBox failmsg(QMessageBox::Warning, "Failed to add tracks", "", QMessageBox::Ok, this);
        failmsg.setText(QString::number(fails.size()) + " tracks could not be added.");
        failmsg.setDetailedText(fails.join("\n"));
        failmsg.exec();
    }
}

void MainWindow::on_actionPlay_triggered() {
    if(!currentPlaylist->empty()) {
        if(!player.currentPlaylist())
            player.openPlaylist(currentPlaylist);
        if(player) {
            if(player.state() == Output::DeviceState::Playing)
                player.pause();
            else if(player.state() != Output::DeviceState::Playing)
                kernel.getThreadManager().enqueue(&Player::play, &player);
        }
    }
}

void MainWindow::on_actionStop_triggered() {
    if(player)
        player.stop();
}

void MainWindow::on_actionNext_triggered() {
    if(player)
        currentPlaylist->next();
}

void MainWindow::on_actionOptions_triggered() {
    ConfigurationDialog c(kernel.getConfigManager());
    c.exec();
}
