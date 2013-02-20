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

namespace LastFM {
class Service;

struct Artist {
    Artist(std::weak_ptr<Service> lastserv);
    Artist(std::weak_ptr<Service> lastserv, const std::string& artist);

    Artist(Artist&&) = default;
    Artist& operator=(Artist&&);
    Artist& operator=(const std::string& artist);

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

}//namespace LastFM

#endif // LASTFM_ARTIST_HPP
