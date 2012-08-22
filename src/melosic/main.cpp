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

//#include <melosic/gui/application.hpp>
//#include <melosic/gui/mainwindow.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/common/file.hpp>
#include <melosic/common/common.hpp>
#include <melosic/core/wav.hpp>
using namespace Melosic;
#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;
using std::ios_base;

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetErrorMode(0x8000);
#endif
//    Application app(argc, argv);

//    MainWindow win;
//    win.show();
//    win.loadPlugins();

//    return app.exec();
    try {
        Kernel kernel;
        kernel.loadPlugin("flac.melin");
        kernel.loadPlugin("alsa.melin");
        IO::File file_i("test.flac");
//        IO::File file_o("test1.wav", ios_base::binary | ios_base::out | ios_base::trunc);
        auto input_ptr = kernel.getInputManager().openFile(file_i);
//        Output::WaveFile output(file_o, input_ptr->getAudioSpecs());

//        io::copy(boost::ref(*input_ptr), boost::ref(output));

        auto device = kernel.getOutputManager().getOutputDevice("iec958:CARD=PCH,DEV=0");

        device->prepareDevice(input_ptr->getAudioSpecs());

        std::clog << device->getSinkName() << std::endl;
        std::clog << device->getDeviceDescription() << std::endl;

        return 0;
    }
    catch(std::exception& e) {
        std::clog << e.what() << std::endl;
        return -1;
    }
}
