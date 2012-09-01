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

#ifndef MELOSIC_PLAYER_HPP
#define MELOSIC_PLAYER_HPP

#include <memory>
#include <chrono>

namespace Melosic {

namespace Input {
class ISource;
}

namespace Output {
class IDeviceSink;
}

class Player
{
public:
    Player(Input::ISource& stream, std::unique_ptr<Output::IDeviceSink> device);
    ~Player();

    void play();
    void pause();
    void stop();
    void seek(std::chrono::milliseconds dur);
    void finish();
    void changeOutput(std::unique_ptr<Output::IDeviceSink> device);
private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}

#endif // MELOSIC_PLAYER_HPP
