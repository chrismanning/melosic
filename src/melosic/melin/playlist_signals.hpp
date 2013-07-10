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

#ifndef MELOSIC_PLAYLIST_SIGNALS_HPP
#define MELOSIC_PLAYLIST_SIGNALS_HPP

#include <memory>

#include <melosic/common/signal_fwd.hpp>

namespace Melosic {

namespace Core {
class Playlist;
class Track;
}

namespace Signals {
namespace Playlist {
typedef SignalCore<void(std::shared_ptr<Core::Playlist>)> PlaylistChanged;
typedef SignalCore<void(const Core::Track&)> TrackChanged;
}
}
}

#endif // MELOSIC_PLAYLIST_SIGNALS_HPP
