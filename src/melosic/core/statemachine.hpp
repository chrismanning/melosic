/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#ifndef MELOSIC_CORE_STATEMACHINE_HPP
#define MELOSIC_CORE_STATEMACHINE_HPP

#include <memory>
#include <chrono>
namespace chrono = std::chrono;

namespace Melosic {

namespace Playlist {
class Manager;
}
namespace Output {
enum class DeviceState;
class Manager;
class PlayerSink;
}

namespace Core {

class Track;
struct StateChanged;

class StateMachine {
public:
    explicit StateMachine(Melosic::Playlist::Manager&, Output::Manager&);
    ~StateMachine();

    void play();
    void pause();
    void stop();

    chrono::milliseconds tell() const;

    Output::DeviceState state() const;
    Output::PlayerSink& sink();

    void sinkChangeSlot();
    void trackChangeSlot(const Track& track);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
    friend struct State;
    friend class Output::Manager;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_CORE_STATEMACHINE_HPP
