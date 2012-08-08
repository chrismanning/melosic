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

#include <melosic/core/kernel.hpp>
#include <melosic/core/wav.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
using boost::iostreams::copy;
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        Melosic::Kernel kernel;
        kernel.loadPlugin("flac.melin");
        kernel.loadPlugin("alsa.melin");
        auto input_ptr = kernel.getInputManager().openFile("test.flac");
        auto& input = *input_ptr;
        auto output = Melosic::Output::WaveFile("test1.wav", input.getAudioSpecs());
//        copy(input, output);

        std::vector<char> buf(4096);
        std::streamsize total = 0;

        while((bool)input) {
            std::streamsize amt;
            amt = io::read(input, buf.data(), 4096);
            if(amt > 0) {
                io::write(output, buf.data(), amt);
                total += amt;
            }
        }

        return 0;
    }
    catch(MelosicException& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
