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

#ifndef LASTFM_TRACK_HPP
#define LASTFM_TRACK_HPP

#include <memory>
#include <list>
#include <future>

#include <melosic/common/range.hpp>
using Melosic::ForwardRange;

#include <network/uri.hpp>

#include "tag.hpp"

namespace Melosic {
namespace Core {
class Track;
}
}

namespace LastFM {
class Service;
struct Artist;
struct Album;

struct Track {
    Track(std::weak_ptr<Service>,
          const std::string& name,
          const std::string& artist,
          const std::string& url);
    Track(std::weak_ptr<Service>, const Melosic::Core::Track&);

    ~Track();

    //field accessors
    ForwardRange<Tag> topTags() const;
    const std::string& getName() const;
    const Artist& getArtist() const;
    const network::uri& getUrl() const;
    const std::string& getWiki() const;

    //network accessors
    std::future<bool> fetchInfo(bool autocorrect = false);
    std::future<bool> scrobble();
    std::future<bool> updateNowPlaying();

private:
    struct impl;
    std::shared_ptr<impl> pimpl;
};


template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>&
operator<<(std::basic_ostream<CharT, TraitsT>& out, const LastFM::Track& track) {
    return out << track.getArtist() << " - " << track.getName() << " : " << track.getUrl();
}

}

#endif // LASTFM_TRACK_HPP
