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
#include <melosic/core/wav.hpp>
using namespace Melosic;
#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;

int main(int argc, char* argv[]) {
//    Application app(argc, argv);

//    MainWindow win;
//    win.show();
//    win.loadPlugins();

//    return app.exec();
    Kernel kernel;
    kernel.loadPlugin("flac.melin");
    IO::File file_i("multitest.flac");
    IO::File file_o("test1.wav");
    auto input_ptr = kernel.getInputManager().openFile(file_i);
    auto& input = *input_ptr;
    Output::WaveFile output(file_o, input.getAudioSpecs());

    io::copy(boost::ref(input), boost::ref(output));

    return 0;
}
