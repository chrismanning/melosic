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

        plugman.addSearchPaths({fs::canonical("../bin"), fs::canonical("../lib")});
        plugman.loadPlugin("flac.melin");
#if __linux
        plugman.loadPlugin("alsa.melin");
#endif
        plugman.loadPlugin("lastfm.melin");

        plugman.initialise();

//        QQmlDebuggingEnabler enabler;

        Core::Player player(kernel.getPlaylistManager(), kernel.getOutputManager());

        kernel.getConfigManager().loadConfig();

        MainWindow win(kernel, player);

        return app.exec();
    }
    catch(PluginVersionMismatchException& e) {
        auto* info = boost::get_error_info<ErrorTag::Plugin::Info>(e);
        assert(info != nullptr);
        std::stringstream str;
        str << info->name << " Plugin API version mismatch. Expected: ";
        str << Plugin::expectedAPIVersion() << "; Got: ";
        str << info->APIVersion;
        ERROR_LOG(logject) << str.str();
        TRACE_LOG(logject) << boost::diagnostic_information(e);
        TRACE_LOG(logject) << info;
        return -1;
    }
    catch(PluginException& e) {
        std::string str("Plugin error");
        if(auto* path = boost::get_error_info<ErrorTag::FilePath>(e)) {
            str += ": " + path->string();
        }
        if(auto* symbol = boost::get_error_info<ErrorTag::Plugin::Symbol>(e)) {
            str += ": undefined symbol: " + *symbol;
        }
        ERROR_LOG(logject) << str;
        TRACE_LOG(logject) << boost::diagnostic_information(e);
        return -1;
    }
    catch(...) {
        TRACE_LOG(logject) << boost::diagnostic_information(boost::current_exception());
    }
}
