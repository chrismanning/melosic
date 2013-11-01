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
#include <boost/optional/optional_fwd.hpp>
using boost::optional;

#include <boost/iostreams/concepts.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/container/stable_vector.hpp>

#include <melosic/common/range.hpp>
#include <melosic/common/common.hpp>
#include <melosic/melin/playlist_signals.hpp>

namespace Melosic {

namespace Decoder {
class Manager;
}

namespace Core {

class Track;

class MELOSIC_EXPORT Playlist {
public:
    struct category : boost::iostreams::input, boost::iostreams::device_tag
    {};
    typedef char char_type;

    typedef Track value_type;
    typedef boost::optional<value_type> optional_type;
    typedef int size_type;

    using Container = boost::container::stable_vector<Playlist::value_type>;

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
    optional_type emplace(size_type pos,
                          boost::filesystem::path filename,
                          chrono::milliseconds start = 0ms,
                          chrono::milliseconds end = 0ms);

    void push_back(value_type value);
    bool emplace_back(boost::filesystem::path filename,
                      chrono::milliseconds start = 0ms,
                      chrono::milliseconds end = 0ms);

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
    friend std::size_t hash_value(const Playlist&);

    Container::iterator begin() const;
    Container::iterator end() const;
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
