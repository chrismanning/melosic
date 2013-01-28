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

#include "tag.hpp"

#include <boost/optional/optional_fwd.hpp>
#include <boost/range/iterator_range.hpp>

#include <opqit/opaque_iterator_fwd.hpp>

#include <memory>
#include <list>

namespace Melosic {
class Track;
}

namespace LastFM {
class Service;
struct Artist;
struct Album;

struct Track {
    Track(std::shared_ptr<Service> lastserv,
          const std::string& name,
          const std::string& artist,
          const std::string& url);
    Track(std::shared_ptr<Service> lastserv, const Melosic::Track&);

    void getInfo(std::function<void(Track&)> callback = [](Track&){}, bool autocorrect = false);
    void scrobble();
//    boost::iterator_range<opqit::opaque_iterator<Track, opqit::forward> >
//    getSimilar(std::shared_ptr<Service> lastserv, unsigned limit = 20);
    boost::iterator_range<opqit::opaque_iterator<Tag, opqit::forward> > topTags();
    const std::string& getName() const;
    const Artist& getArtist() const;
    const network::uri& getUrl() const;
    const std::string& getWiki() const;

    template <typename CharT, typename TraitsT>
    friend std::basic_ostream<CharT, TraitsT>&
    operator<<(std::basic_ostream<CharT, TraitsT>& out, const LastFM::Track& track);

private:
    struct impl;
    std::shared_ptr<impl> pimpl;
};

}

#endif // LASTFM_TRACK_HPP
