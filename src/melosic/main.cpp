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
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        Melosic::Kernel kernel;
        kernel.loadPlugin("flac.melin");
        kernel.loadPlugin("alsa.melin");
        return 0;
    }
    catch(MelosicException& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
