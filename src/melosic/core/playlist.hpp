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
#include <boost/iostreams/concepts.hpp>
#include <boost/signals2.hpp>
#include <chrono>
namespace chrono = std::chrono;
#include <boost/range/iterator_range.hpp>

#include <opqit/opaque_iterator.hpp>

namespace Melosic {

class Track;

class Playlist {
public:
    typedef boost::iostreams::input_seekable category;
    typedef char char_type;

    typedef Track value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef opqit::opaque_iterator<value_type, opqit::random> iterator;
    typedef opqit::opaque_iterator<const value_type, opqit::random> const_iterator;
    typedef boost::iterator_range<iterator> range;
    typedef boost::iterator_range<const_iterator> const_range;
    typedef int size_type;

    Playlist();
    ~Playlist();

    std::streamsize read(char* s, std::streamsize n);
    void seek(chrono::milliseconds dur);
    chrono::milliseconds duration() const;
    void previous();
    void next();
    iterator& currentTrack();
    const_iterator currentTrack() const;

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
    explicit operator bool() {
        return currentTrack() != end();
    }

    iterator insert(iterator pos, value_type&& value);
    void insert(iterator pos, iterator first, iterator last);

    void push_back(value_type&& value);
    template <typename ... Args>
    void emplace_back(Args&& ... args) {
        emplace(end(), std::forward<Args>(args)...);
    }

    template <typename ... Args>
    iterator emplace(iterator pos, Args&& ... args) {
        return insert(pos, std::move(value_type(std::forward<Args>(args)...)));
    }

//    template <typename It>
//    iterator insert(const_iterator pos, It first, It last);
//    template<class ... Args>
//    iterator erase(const_iterator pos);
//    iterator erase(const_iterator first, const_iterator last);
//    void clear();
    void swap(Playlist& b);

    //signals
    typedef boost::signals2::signal<void(const Track&, bool)> TrackChangedSignal;
    boost::signals2::connection connectTrackChangedSlot(const TrackChangedSignal::slot_type& slot);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
extern template class opqit::opaque_iterator<Melosic::Track, opqit::random>;

#endif // MELOSIC_PLAYLIST_HPP
