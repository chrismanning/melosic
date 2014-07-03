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

#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QApplication>
#include <QVector>
#include <QItemSelectionModel>

#include <boost/log/sinks/sync_frontend.hpp>

#include <melosic/common/common.hpp>
#include <melosic/melin/kernel.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/optional.hpp>
#include <melosic/melin/library.hpp>

#include <jbson/json_writer.hpp>
#include <jbson/builder.hpp>
#include <jbson/json_reader.hpp>

#include "mainwindow.hpp"
#include "playlistmodel.hpp"
#include "playlistmanagermodel.hpp"
#include "playercontrols.hpp"
#include "categoryproxymodel.hpp"
#include "category.hpp"
#include "categorytag.hpp"
#include "quicklogbackend.hpp"
#include "filterpane.hpp"
#include "jsondocmodel.hpp"
#include "configmanager.hpp"
#include "librarymanager.hpp"

static Melosic::Config::Conf conf{"QML"};
static bool enable_logging{false};

namespace Melosic {

using namespace jbson::literal;

MainWindow::MainWindow(Core::Kernel& kernel, Core::Player& player) :
    logject(logging::keywords::channel = "MainWindow"),
    engine(new QQmlEngine),
    component(new QQmlComponent(engine.get())),
    playlistManagerModel(new PlaylistManagerModel(kernel.getPlaylistManager(), kernel.getDecoderManager(), kernel.getLibraryManager()))
{
    ::conf.putNode("enable logging", ::enable_logging);
    scopedSigConns.emplace_back(player.stateChangedSignal().connect(&MainWindow::onStateChangeSlot, this));

    playerControls.reset(new PlayerControls(player, kernel.getPlaylistManager()));

    LibraryManager::instance()->setLibraryManager(kernel.getLibraryManager());

    //register types for use in QML
    qmlRegisterType<Block>("Melosic.Playlist", 1, 0, "Block");
    qmlRegisterType<QAbstractItemModel>();
    qmlRegisterType<QItemSelectionModel>();
    qmlRegisterType<PlaylistModel>();
    qmlRegisterType<QuickLogBackend>();
    qRegisterMetaType<QVector<int>>();
    qmlRegisterType<CategoryProxyModel>("Melosic.Playlist", 1, 0, "CategoryProxyModel");
    qmlRegisterUncreatableType<Criterion>("Melosic.Playlist", 1, 0, "CategoryCriteria", "abstract");
    qmlRegisterType<Role>("Melosic.Playlist", 1, 0, "CategoryRole");
    qmlRegisterType<Category>("Melosic.Playlist", 1, 0, "Category");
    qmlRegisterType<CategoryTag>("Melosic.Playlist", 1, 0, "CategoryTag");
    qmlRegisterType<TagBinding>("Melosic.Playlist", 1, 0, "TagBinding");
    qmlRegisterType<FilterPane>("Melosic.Browser", 1, 0, "FilterPane");
    qmlRegisterUncreatableType<SelectionModel>("Melosic.Browser", 1, 0, "SelectionModel", "C++ only");

    if(::enable_logging) {
        auto sink = boost::make_shared<logging::sinks::synchronous_sink<QuickLogBackend>>();
        logging::core::get()->add_sink(sink);
        engine->rootContext()->setContextProperty("logsink", sink->locked_backend().get());
    }
    engine->rootContext()->setContextProperty("enable_logging", ::enable_logging);

    engine->rootContext()->setContextProperty("playlistManagerModel", playlistManagerModel);
    engine->rootContext()->setContextProperty("PlayerControls", playerControls.get());
    engine->rootContext()->setContextProperty("LibraryManager", LibraryManager::instance());

    ConfigManager::instance()->setConfigManager(kernel.getConfigManager());
    engine->rootContext()->setContextProperty("ConfigManager", ConfigManager::instance());
    engine->addImportPath("qrc:/");
    engine->addImportPath("qrc:/qml");

    component->loadUrl(QUrl("qrc:/qml/mainwindow.qml"));

    if(!component->isReady()) {
        ERROR_LOG(logject) << qPrintable(component->errorString());
        BOOST_THROW_EXCEPTION(Exception() << ErrorTag::DecodeErrStr(qPrintable(component->errorString())));
    }
    QObject* topLevel = component->create();
    if(component->isError()) {
        ERROR_LOG(logject) << qPrintable(component->errorString());
        BOOST_THROW_EXCEPTION(Exception() << ErrorTag::DecodeErrStr(qPrintable(component->errorString())));
    }
    window.reset(qobject_cast<QQuickWindow*>(topLevel));
    if(!window) {
        std::string str("Error: Your root item has to be a Window.");
        ERROR_LOG(logject) << str;
        ERROR_LOG(logject) << "Type: " << topLevel->metaObject()->className();
        BOOST_THROW_EXCEPTION(Exception() << ErrorTag::DecodeErrStr(str));
    }
    ConfigManager::instance()->setParent(window.get());
//    QObject::connect(engine.data(), &QQmlEngine::quit, window.data(), &QQuickWindow::close);
    QObject::connect(engine.get(), SIGNAL(quit()), qApp, SLOT(quit()), Qt::DirectConnection);
    window->show();
}

MainWindow::~MainWindow() {
//    TRACE_LOG(logject) << "Destroying main window";
}

void MainWindow::onStateChangeSlot(Output::DeviceState /*state*/) {
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

void MainWindow::scanEndedSlot() {

}

} // namespace Melosic
