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
namespace chrono = std::chrono;

#include <boost/iostreams/concepts.hpp>
#include <boost/filesystem/path.hpp>

#include <opqit/opaque_iterator.hpp>

#include <melosic/common/range.hpp>

namespace Melosic {

namespace Slots {
class Manager;
}

namespace Decoder {
class Manager;
}

namespace Core {

class Track;

class Playlist {
public:
    typedef boost::iostreams::input_seekable category;
    typedef char char_type;

    typedef Track value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef RandomRange<value_type> range;
    typedef RandomRange<const value_type> const_range;
    typedef ForwardRange<value_type> forward_range;
    typedef ForwardRange<const value_type> const_forward_range;
    typedef opqit::opaque_iterator<value_type, opqit::random> iterator;
    typedef opqit::opaque_iterator<const value_type, opqit::random> const_iterator;
    typedef int size_type;

    Playlist(const std::string&, Slots::Manager&, Decoder::Manager&);
    ~Playlist();

    std::streamsize read(char* s, std::streamsize n);
    void seek(chrono::milliseconds dur);
    chrono::milliseconds duration() const;
    void previous();
    void next();
    void jumpTo(size_type pos);
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
    explicit operator bool() const {
        return currentTrack() != end();
    }

    iterator insert(const_iterator pos, value_type&& value);
    void insert(const_iterator pos, forward_range values);
    iterator emplace(const_iterator pos,
                     const boost::filesystem::path& filename,
                     chrono::milliseconds start = chrono::milliseconds(0),
                     chrono::milliseconds end = chrono::milliseconds(0));
    iterator emplace(const_iterator pos, ForwardRange<const boost::filesystem::path> values);

    void push_back(value_type&& value);
    void emplace_back(const boost::filesystem::path& filename,
                      chrono::milliseconds start = chrono::milliseconds(0),
                      chrono::milliseconds end = chrono::milliseconds(0));

    iterator erase(const_iterator pos);
    iterator erase(size_type start, size_type end);
    void erase(forward_range values);
    void clear();
    void swap(Playlist& b);

    const std::string& getName() const;
    void setName(const std::string&);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic
extern template class opqit::opaque_iterator<Melosic::Core::Track, opqit::random>;

#endif // MELOSIC_PLAYLIST_HPP
