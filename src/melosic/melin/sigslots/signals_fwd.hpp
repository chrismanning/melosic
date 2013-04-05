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

#ifndef MELOSIC_SIGNALS_FWD_HPP
#define MELOSIC_SIGNALS_FWD_HPP

#include <memory>
#include <string>

#include <melosic/common/common.hpp>
#include <melosic/melin/configvar.hpp>

namespace Melosic {
namespace Core {
class Playlist;
class Track;
}

namespace Config {
class Base;
}
namespace Output {
enum class DeviceState;
}
namespace Signals {

template <typename Ret, typename ...Args>
class Signal;

namespace Config {
typedef Signal<void(const std::string&, const Melosic::Config::VarType&)> VariableUpdated;
typedef Signal<void(Melosic::Config::Base&)> Loaded;
}

namespace Player {
typedef Signal<void(Output::DeviceState)> StateChanged;
typedef Signal<void(chrono::milliseconds, chrono::milliseconds)> NotifyPlayPos;
typedef Signal<void()> Play;
typedef Signal<void()> Pause;
typedef Signal<void()> Stop;
typedef Signal<void(chrono::milliseconds)> Seek;
}

namespace Output {
typedef Signal<void(const std::string&)> ReqSinkChange;
typedef Signal<void()> PlayerSinkChanged;
}

namespace Playlist {
typedef Signal<void(std::shared_ptr<Melosic::Core::Playlist>)> PlaylistChanged;
typedef Signal<void(const Core::Track&)> TrackChanged;
typedef Signal<void()> Next;
typedef Signal<void()> Previous;
}

} // namespace Signals
} // namespace Melosic

#endif // MELOSIC_SIGNALS_FWD_HPP
