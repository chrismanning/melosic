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

#ifndef LASTFM_ARTIST_HPP
#define LASTFM_ARTIST_HPP

#include <list>
#include <memory>

#include "utilities.hpp"
#include "tag.hpp"

namespace LastFM {
class Service;

struct Artist {
    Artist() = default;
    Artist(const std::string& artist) : name(artist) {}
    Artist(const std::string& artist, const std::string& url) : name(artist), url(url) {}
    Artist& operator=(const Artist& artist);
    Artist& operator=(const std::string& artist);

    void getInfo(std::shared_ptr<Service> lastserv, bool autocorrect = false);
    boost::iterator_range<opqit::opaque_iterator<Artist, opqit::forward>> getSimilar(std::shared_ptr<Service> lastserv);

    std::string name;
    network::uri url;
    std::string biographySummary, biography;
    bool streamable;
    std::list<Tag> tags;
};

}//namespace LastFM

#endif // LASTFM_ARTIST_HPP
