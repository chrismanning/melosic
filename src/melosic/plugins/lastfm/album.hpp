/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#ifndef LASTFM_ALBUM_HPP
#define LASTFM_ALBUM_HPP

#include <string>
#include <vector>

#include <network/uri.hpp>

#include "artist.hpp"
#include "tag.hpp"
#include "track.hpp"

namespace lastfm {

struct album {

private:
    std::string m_album_name;
    artist m_artist;
    network::uri m_url;
    std::vector<track> m_tracks;
    std::vector<tag> m_top_tags;
    int m_listeners;
    int m_play_count;
};

} // namespace lastfm

#endif // LASTFM_ALBUM_HPP
