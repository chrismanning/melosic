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
using namespace std::literals;

#include <boost/filesystem/path.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/detail/any_iterator.hpp>

#include <melosic/common/range.hpp>
#include <melosic/common/common.hpp>
#include <melosic/melin/playlist_signals.hpp>
#include <melosic/common/optional_fwd.hpp>

namespace Melosic {

namespace Decoder {
class Manager;
}

namespace Core {

class Track;

class MELOSIC_EXPORT Playlist {
public:
    typedef Track value_type;
    typedef optional<value_type> optional_type;
    typedef int size_type;

    Playlist(Decoder::Manager&, std::string);
    ~Playlist();

    chrono::milliseconds duration() const;

    optional_type getTrack(size_type);
    const optional_type getTrack(size_type) const;

    std::vector<value_type> getTracks(size_type, size_type);
    const std::vector<value_type> getTracks(size_type, size_type) const;

    void refreshTracks(size_type, size_type);

    bool empty() const;
    size_type size() const;
    size_type max_size() const;

    size_type insert(size_type pos, ForwardRange<value_type> values);
    size_type emplace(size_type pos, ForwardRange<const boost::filesystem::path> values);
    value_type insert(size_type pos, value_type value);
    size_type emplace(size_type pos, boost::filesystem::path filename);

    void push_back(value_type value);
    bool emplace_back(boost::filesystem::path filename);

    void erase(size_type pos);
    void erase(size_type start, size_type end);

    void clear();
    void swap(Playlist& b);

    const std::string& getName() const;
    void setName(std::string);

    bool operator==(const Playlist&) const;

    Signals::Playlist::TagsChanged& getTagsChangedSignal() const noexcept;
    Signals::Playlist::MultiTagsChanged& getMutlipleTagsChangedSignal() const noexcept;

private:
    class impl;
    std::shared_ptr<impl> pimpl;
    struct Iterator;
    friend struct Iterator;
    friend std::size_t hash_value(const Playlist&);

    typedef boost::range_detail::any_iterator<optional_type,
                                              boost::random_access_traversal_tag,
                                              optional_type,
                                              ptrdiff_t> iterator;

    iterator begin() const;
    iterator end() const;
    friend class Player;
};

inline std::size_t hash_value(const Playlist& b) {
    return std::hash<std::shared_ptr<Melosic::Core::Playlist::impl>>()(b.pimpl);
}

} // namespace Core
} // namespace Melosic

namespace std {
    template <>
    struct hash<Melosic::Core::Playlist> {
        std::size_t operator()(Melosic::Core::Playlist const& s) const {
            return hash_value(s);
        }
    };
}

#endif // MELOSIC_PLAYLIST_HPP
