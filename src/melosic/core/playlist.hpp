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

#ifndef MELOSIC_PLAYLIST_HPP
#define MELOSIC_PLAYLIST_HPP

#include <memory>
#include <chrono>
#include <deque>
#include <boost/iostreams/concepts.hpp>

namespace Melosic {

class Track;

class Playlist
{
public:
    typedef boost::iostreams::input_seekable category;
    typedef char char_type;
    typedef Track value_type;
    typedef std::deque<value_type> list_type;
    typedef list_type::iterator iterator;
    typedef list_type::const_iterator const_iterator;

    Playlist();
    ~Playlist();
    std::streamsize read(char * s, std::streamsize n);
    std::streampos seek(std::streamoff off, std::ios_base::seekdir way);
    void seek(std::chrono::milliseconds dur);

    iterator begin();
    const_iterator cbegin();
    iterator end();
    const_iterator cend();
    iterator insert(const_iterator pos, const value_type& value);
    iterator insert(const_iterator pos, value_type&& value);
    template <typename It>
    iterator insert(const_iterator pos, It first, It last);
    void clear();
private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}

#endif // MELOSIC_PLAYLIST_HPP
