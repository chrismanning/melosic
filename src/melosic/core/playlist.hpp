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
#include <list>
#include <deque>
#include <mutex>
#include <thread>
#include <type_traits>
#include <boost/iostreams/concepts.hpp>
#include <boost/signals2.hpp>

namespace Melosic {

class Track;

class Playlist
{
public:
    typedef boost::iostreams::input_seekable category;
    typedef char char_type;

    typedef std::deque<Track> list_type;
    typedef list_type::value_type value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef list_type::iterator iterator;
    typedef list_type::const_iterator const_iterator;
    typedef list_type::size_type size_type;

    Playlist();
    Playlist(const Playlist& b);
    Playlist& operator=(const Playlist& b);
    Playlist& operator=(Playlist&& b);
    ~Playlist();
    std::streamsize read(char * s, std::streamsize n);
    void seek(std::chrono::milliseconds dur);
    std::chrono::milliseconds duration();
    void previous();
    void next();
    iterator& current();
    const_iterator current() const;

    reference operator[](size_type pos);
    const_reference operator[](size_type pos) const;
    reference front();
    const_reference front() const;
    reference back();
    const_reference back() const;

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

    bool empty() const;
    size_type size() const;
    size_type max_size() const;

    iterator insert(iterator pos, const value_type& value);
    iterator insert(iterator pos, value_type&& value);
    void push_back(const value_type& value);
    void push_back(value_type&& value);
    template<class ... Args>
    void emplace_back(Args&& ... args) {
        tracks.emplace_back(std::forward<Args>(args)...);
    }

//    template <typename It>
//    iterator insert(const_iterator pos, It first, It last);
//    iterator insert(const_iterator pos, std::initializer_list<value_type> ilist);
//    template<class ... Args>
//    iterator emplace(const_iterator pos, Args&& ... args);
//    iterator erase(const_iterator pos);
//    iterator erase(const_iterator first, const_iterator last);
//    void clear();
    void swap(Playlist& b) {
        std::swap(tracks, b.tracks);
    }

    //signals
    typedef boost::signals2::signal<void()> TrackChangedSignal;
    boost::signals2::connection connectTrackChanged(const TrackChangedSignal::slot_type& slot) {
        return trackChanged.connect(slot);
    }

private:
    template <class It>
    reference getTrack(size_type pos, It beg, std::random_access_iterator_tag) {
        return beg[pos];
    }
    template <class It>
    reference getTrack(size_type pos, It beg, std::bidirectional_iterator_tag) {
        std::advance(beg, pos);
        return *beg;
    }

    template <class It>
    const_reference getTrack(size_type pos, It beg, std::random_access_iterator_tag) const {
        return beg[pos];
    }
    template <class It>
    const_reference getTrack(size_type pos, It beg, std::bidirectional_iterator_tag) const {
        std::advance(beg, pos);
        return *beg;
    }

    list_type tracks;
    iterator current_track;
    TrackChangedSignal trackChanged;
    std::mutex mu;
    typedef decltype(mu) Mutex;
};

}

#endif // MELOSIC_PLAYLIST_HPP
