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

#include <melosic/gui/application.hpp>
#include <melosic/gui/mainwindow.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/common/file.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/plugin.hpp>
using namespace Melosic;

#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;
using std::ios_base;

static Logger::Logger logject(boost::log::keywords::channel = "Main");

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetErrorMode(0x8000);
#endif

    try {
        Logger::init();

        QApplication app(argc, argv);

        Kernel& kernel = Kernel::getInstance();

        kernel.loadPlugin("flac.melin");
        kernel.loadPlugin("alsa.melin");
        kernel.loadPlugin("lastfm.melin");

        MainWindow win;
        win.show();

        return app.exec();
    }
    catch(PluginVersionMismatch& e) {
        auto* info = boost::get_error_info<ErrorTag::Plugin::Info>(e);
        assert(info != nullptr);
        std::stringstream str;
        str << info->name << " Plugin API version mismatch. Expected: ";
        str << Plugin::expectedAPIVersion() << "; Got: ";
        str << info->APIVersion;
        ERROR_LOG(logject) << str.str();
        TRACE_LOG(logject) << boost::diagnostic_information(e);
        TRACE_LOG(logject) << info;
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
    }
    catch(boost::exception& e) {
        TRACE_LOG(logject) << boost::diagnostic_information(e);
    }
    catch(std::exception& e) {
        ERROR_LOG(logject) << e.what();
        return -1;
    }

}
