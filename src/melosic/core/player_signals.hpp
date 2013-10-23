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

#ifndef MELOSIC_STATECHANGED_HPP
#define MELOSIC_STATECHANGED_HPP

#include <chrono>
namespace chrono = std::chrono;

#include <melosic/common/signal_fwd.hpp>

namespace Melosic {

namespace Output {
enum class DeviceState;
}

namespace Signals {

namespace Player {
typedef SignalCore<void(Melosic::Output::DeviceState)> StateChanged;
typedef SignalCore<void(chrono::milliseconds /*current*/, chrono::milliseconds /*total*/)> NotifyPlayPos;
}

}

}

#endif // MELOSIC_STATECHANGED_HPP
