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

#ifndef LASTFM_TAG_HPP
#define LASTFM_TAG_HPP

#include <string>

#include <network/uri.hpp>

namespace LastFM {

class Service;

struct Tag {
    Tag(const std::string& tag, const std::string& url) : name(tag), url(url) {}
    Tag& operator=(const std::string& tag);

    std::string name;
    network::uri url;
    int reach = 0;
    int taggings = 0;
    bool streamable = false;
    std::string wiki;
};

}//namespace LastFM

#endif // LASTFM_TAG_HPP
