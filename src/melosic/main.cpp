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
#include <melosic/common/common.hpp>
#include <melosic/core/wav.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/track.hpp>
using namespace Melosic;

#include <thread>
#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;
using std::ios_base;

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetErrorMode(0x8000);
#endif

    try {
        QApplication app(argc, argv);

        Kernel kernel;
        kernel.loadPlugin("flac.melin");
        kernel.loadPlugin("alsa.melin");

        MainWindow win(kernel);
        win.show();

        return app.exec();
    }
    catch(std::exception& e) {
        std::clog << e.what() << std::endl;
        return -1;
    }

}
