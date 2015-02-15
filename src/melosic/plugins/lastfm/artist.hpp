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

#include <memory>
#include <future>

#include <network/uri.hpp>

namespace lastfm {
class service;

struct artist {
    artist(std::weak_ptr<service> lastserv);
    artist(std::weak_ptr<service> lastserv, const std::string& artist);

    artist(artist&&) = default;
    artist& operator=(artist&&);
    artist& operator=(const std::string& artist);

    explicit operator bool();

    //field accessors
    const std::string& getName() const;
    const network::uri& getUrl() const;

    //network accessors
    std::future<bool> fetchInfo(bool autocorrect = false);

private:
    struct impl;
    std::shared_ptr<impl> pimpl;
};

}//namespace lastfm

#endif // LASTFM_ARTIST_HPP
