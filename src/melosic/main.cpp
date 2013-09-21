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

#include <QApplication>
#include <QQmlDebuggingEnabler>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <melosic/melin/kernel.hpp>
#include <melosic/common/file.hpp>
#include <melosic/common/error.hpp>
#include <melosic/melin/exports.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/gui/mainwindow.hpp>
#include <melosic/core/player.hpp>
using namespace Melosic;

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetErrorMode(0x8000);
#endif

    boost::locale::generator gen;
    std::locale::global(gen("en_GB.UTF-8"));
    Logger::Logger logject(logging::keywords::channel = "Main");

    try {
        Logger::init();

        QApplication app(argc, argv);

        Core::Kernel kernel;
        Plugin::Manager& plugman = kernel.getPluginManager();

        Core::Player player(kernel);

        kernel.getConfigManager().loadConfig();

        plugman.loadPlugins(kernel);

//        QQmlDebuggingEnabler enabler;

        MainWindow win(kernel, player);

        return app.exec();
    }
    catch(...) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
    }
    return -1;
}
