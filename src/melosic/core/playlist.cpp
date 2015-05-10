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

#include <numeric>
#include <thread>
#include <mutex>

#include <shared_mutex>
#include <boost/thread/locks.hpp>
using mutex = std::shared_timed_mutex;
using lock_guard = std::lock_guard<mutex>;
using unique_lock = boost::unique_lock<mutex>;
using shared_lock = std::shared_lock<mutex>;
#include <boost/range/adaptor/sliced.hpp>
#include <boost/container/stable_vector.hpp>
#include <boost/thread/reverse_lock.hpp>

#include <taglib/tpropertymap.h>

#include <melosic/core/track.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/common/audiospecs.hpp>

#include "playlist.hpp"

namespace Melosic {
namespace Core {

static Logger::Logger logject(logging::keywords::channel = "Playlist");

struct TrackAdded : Signals::Signal<Signals::Playlist::TrackAdded> {
    using Super::Signal;
};

struct TrackRemoved : Signals::Signal<Signals::Playlist::TrackRemoved> {
    using Super::Signal;
};

struct TagsChanged : Signals::Signal<Signals::Playlist::TagsChanged> {
    using Super::Signal;
};

struct MultiTagsChanged : Signals::Signal<Signals::Playlist::MultiTagsChanged> {
    using Super::Signal;
};

class Playlist::impl : public std::enable_shared_from_this<impl> {
  public:
    using Container = boost::container::stable_vector<Playlist::value_type>;

    impl(std::string name) : name(name) {
    }

    void trackAdded(Playlist::size_type i, optional<Core::Track> t, unique_lock& l) {
        if(i < 0 || !t)
            return;
        t->getTagsChangedSignal().connect([ it = std::next(std::begin(tracks), i), this ](
            const boost::synchronized_value<TagMap>& tags) mutable {
            //            tagsChangedSignal(+std::distance(std::begin(tracks), it), std::cref(tags));
        });
        boost::reverse_lock<unique_lock> s{l};
        trackAddedSignal(i, t);
    }

    void trackRemoved(Playlist::size_type i, optional<Core::Track> t, unique_lock& l) {
        boost::reverse_lock<unique_lock> s{l};
        trackRemovedSignal(i, t);
    }

    void tagsChanged(Playlist::size_type i, const TagLib::PropertyMap& tags, unique_lock& l) {
        boost::reverse_lock<unique_lock> s{l};
        tagsChangedSignal(i, std::cref(tags));
    }

    // playlist controls
    chrono::milliseconds duration() {
        return std::accumulate(std::begin(tracks), std::end(tracks), 0ms,
                               [](chrono::milliseconds a, Track& b) { return a + b.duration(); });
    }

    // capacity
    bool empty() {
        return tracks.empty();
    }

    Playlist::size_type size() {
        return tracks.size();
    }

    Playlist::size_type max_size() const {
        return std::numeric_limits<Playlist::size_type>::max();
    }

    auto insert(Playlist::size_type pos, Playlist::value_type value, unique_lock& l) {
        if(pos > size())
            pos = size();
        auto r = tracks.insert(std::next(std::begin(tracks), pos), std::move(value));

        trackAdded(pos, *r, l);

        return +std::distance(std::begin(tracks), r);
    }

    Playlist::size_type insert(Playlist::size_type pos, ForwardRange<Playlist::value_type> values, unique_lock& l) {
        if(pos > size())
            pos = size();

        const auto old_size = size();
        Playlist::size_type s{0};

        if(values.empty())
            return s;

        for(const auto& val : values) {
            insert(pos, val, l);
            pos++;
            s++;
        }
        assert(size() == old_size + s);

        return s;
    }

    void push_back(Playlist::value_type value, unique_lock& l) {
        tracks.push_back(std::move(value));

        trackAdded(tracks.size() - 1, tracks.back(), l);
    }

    void erase(Playlist::size_type pos) {
        if(pos >= size())
            return;
        tracks.erase(std::next(std::begin(tracks), pos));
    }

    void erase(Playlist::size_type start, Playlist::size_type end) {
        tracks.erase(std::next(std::begin(tracks), start), std::next(std::begin(tracks), end));
    }

    void clear() {
        tracks.clear();
    }

    const std::string& getName() {
        return name;
    }

    void setName(std::string name) {
        TRACE_LOG(logject) << "setting playlist name to \"" << name << "\"";
        this->name.swap(name);
    }

    mutex mu;
    Container tracks;
    TrackAdded trackAddedSignal;
    TrackRemoved trackRemovedSignal;
    TagsChanged tagsChangedSignal;
    MultiTagsChanged multiTagsChangedSignal;

    std::string name;
};

Playlist::Playlist(std::string name) : pimpl(std::make_shared<impl>(std::move(name))) {
}

Playlist::~Playlist() {
}

// playlist controls
chrono::milliseconds Playlist::duration() const {
    shared_lock l(pimpl->mu);
    return pimpl->duration();
}

Playlist::optional_type Playlist::getTrack(size_type pos) {
    shared_lock l(pimpl->mu);
    if(pos >= pimpl->size())
        return {};
    return {pimpl->tracks[pos]};
}

const Playlist::optional_type Playlist::getTrack(size_type pos) const {
    shared_lock l(pimpl->mu);
    if(pos >= pimpl->size())
        return {};
    return {pimpl->tracks[pos]};
}

std::vector<Playlist::value_type> Playlist::getTracks(Playlist::size_type start, Playlist::size_type end) {
    shared_lock l(pimpl->mu);
    auto range = boost::adaptors::slice(pimpl->tracks, start, end);
    return {range.begin(), range.end()};
}

const std::vector<Playlist::value_type> Playlist::getTracks(Playlist::size_type start, Playlist::size_type end) const {
    shared_lock l(pimpl->mu);
    auto range = boost::adaptors::slice(pimpl->tracks, start, end);
    return {range.begin(), range.end()};
}

void Playlist::refreshTracks(Playlist::size_type start, Playlist::size_type end) {
    unique_lock l(pimpl->mu);
    auto range = boost::adaptors::slice(pimpl->tracks, start, end);
    for(auto& t : range) {
        try {
            //            t.reOpen();
            //            t.close();
        } catch(...) {
        }
    }
    l.unlock();
    pimpl->multiTagsChangedSignal(start, end);
}

// capacity
bool Playlist::empty() const {
    shared_lock l(pimpl->mu);
    return pimpl->empty();
}

Playlist::size_type Playlist::size() const {
    shared_lock l(pimpl->mu);
    return pimpl->size();
}

Playlist::size_type Playlist::max_size() const {
    return std::numeric_limits<size_type>::max();
}

size_t Playlist::insert(size_type pos, Playlist::value_type value) {
    unique_lock l(pimpl->mu);
    return pimpl->insert(pos, std::move(value), l);
}

Playlist::size_type Playlist::insert(size_type pos, ForwardRange<value_type> values) {
    unique_lock l(pimpl->mu);
    return pimpl->insert(pos, values, l);
}

void Playlist::push_back(Playlist::value_type value) {
    unique_lock l(pimpl->mu);
    pimpl->push_back(std::move(value), l);
}

void Playlist::erase(size_type pos) {
    lock_guard l(pimpl->mu);
    pimpl->erase(pos);
}

void Playlist::erase(size_type start, size_type end) {
    lock_guard l(pimpl->mu);
    return pimpl->erase(start, end);
}

void Playlist::clear() {
    lock_guard l(pimpl->mu);
    pimpl->clear();
}

void Playlist::swap(Playlist& b) {
    unique_lock l{pimpl->mu, boost::defer_lock}, l2{b.pimpl->mu, boost::defer_lock};
    std::lock(l, l2);
    std::swap(pimpl, b.pimpl);
}

std::string_view Playlist::name() const {
    shared_lock l(pimpl->mu);
    return pimpl->getName();
}

void Playlist::name(std::string name) {
    lock_guard l(pimpl->mu);
    pimpl->setName(std::move(name));
}

bool Playlist::operator==(const Playlist& b) const {
    return pimpl == b.pimpl;
}

bool Playlist::operator!=(const Playlist& b) const {
    return !(*this == b);
}

Signals::Playlist::TagsChanged& Playlist::getTagsChangedSignal() const noexcept {
    return pimpl->tagsChangedSignal;
}

Signals::Playlist::MultiTagsChanged& Playlist::getMutlipleTagsChangedSignal() const noexcept {
    return pimpl->multiTagsChangedSignal;
}

template <typename Value>
struct Playlist::Iterator : boost::iterator_facade<Iterator<Value>, Value, boost::random_access_traversal_tag> {
    static_assert(!std::is_reference<Value>::value, "");

    Iterator() = default;
    Iterator(const Iterator&) = default;

    template <typename OtherValue, typename = std::enable_if_t<std::is_convertible<OtherValue*, Value*>::value>>
    Iterator(const Iterator<OtherValue>& b)
        : m_playlist_pimpl(b.m_playlist_pimpl), m_iterator(b.m_iterator), m_track(b.m_track) {
    }

    explicit Iterator(std::shared_ptr<impl> playlist);
    Iterator(std::shared_ptr<impl> playlist, Playlist::impl::Container::iterator it);

  private:
    Value& dereference() const;
    template <class OtherValue> bool equal(const Iterator<OtherValue>& b) const;
    void increment();
    void decrement();
    void advance(std::ptrdiff_t n);
    template <class OtherValue> std::ptrdiff_t distance_to(const Iterator<OtherValue>& b) const;

    friend class boost::iterator_core_access;
    std::shared_ptr<Playlist::impl> m_playlist_pimpl;
    Playlist::impl::Container::iterator m_iterator;
    mutable optional<std::decay_t<Value>> m_track;
};

Playlist::const_iterator Playlist::begin() const {
    shared_lock l(pimpl->mu);
    return Playlist::const_iterator{Iterator<const value_type>{pimpl, pimpl->tracks.begin()}};
}

Playlist::const_iterator Playlist::end() const {
    shared_lock l(pimpl->mu);
    return Playlist::const_iterator{Iterator<const value_type>{pimpl, pimpl->tracks.end()}};
}

Playlist::iterator Playlist::begin() {
    shared_lock l(pimpl->mu);
    return Playlist::iterator{Iterator<value_type>{pimpl, pimpl->tracks.begin()}};
}

Playlist::iterator Playlist::end() {
    shared_lock l(pimpl->mu);
    return Playlist::iterator{Iterator<value_type>{pimpl, pimpl->tracks.end()}};
}

template <typename Value>
Playlist::Iterator<Value>::Iterator(std::shared_ptr<impl> playlist)
    : m_playlist_pimpl(playlist), m_iterator(playlist->tracks.end()) {
    assert(m_playlist_pimpl);
    if(m_iterator == m_playlist_pimpl->tracks.end() || m_playlist_pimpl->tracks.empty())
        return;
    m_track = *m_iterator;
}

template <typename Value>
Playlist::Iterator<Value>::Iterator(std::shared_ptr<impl> playlist, Playlist::impl::Container::iterator it)
    : m_playlist_pimpl(playlist), m_iterator(it) {
    assert(m_playlist_pimpl);
    shared_lock l(m_playlist_pimpl->mu);
    if(m_iterator == m_playlist_pimpl->tracks.end() || m_playlist_pimpl->tracks.empty())
        return;
    m_track = *m_iterator;
}

template <typename Value> Value& Playlist::Iterator<Value>::dereference() const {
    assert(m_playlist_pimpl);
    return *m_track;
}

template <typename Value>
template <class OtherValue>
bool Playlist::Iterator<Value>::equal(const Playlist::Iterator<OtherValue>& b) const {
    assert(m_playlist_pimpl);
    shared_lock l(m_playlist_pimpl->mu);
    return b.m_iterator == m_iterator;
}

template <typename Value> void Playlist::Iterator<Value>::increment() {
    assert(m_playlist_pimpl);
    shared_lock l(m_playlist_pimpl->mu);
    if(m_playlist_pimpl->tracks.end() == m_iterator)
        return;
    m_iterator++;
    if(m_playlist_pimpl->tracks.end() == m_iterator)
        m_track = nullopt;
    else
        m_track = *m_iterator;
}

template <typename Value> void Playlist::Iterator<Value>::decrement() {
    assert(m_playlist_pimpl);
    shared_lock l(m_playlist_pimpl->mu);
    if(m_playlist_pimpl->tracks.begin() == m_iterator)
        return;
    m_track = *--m_iterator;
}

template <typename Value> void Playlist::Iterator<Value>::advance(std::ptrdiff_t n) {
    assert(m_playlist_pimpl);
    shared_lock l(m_playlist_pimpl->mu);
    if(n >= std::distance(m_iterator, m_playlist_pimpl->tracks.end())) {
        m_iterator = m_playlist_pimpl->tracks.end();
        m_track = nullopt;
    } else
        m_track = *(m_iterator = std::next(m_iterator, n));
}

template <typename Value>
template <class OtherValue>
std::ptrdiff_t Playlist::Iterator<Value>::distance_to(const Playlist::Iterator<OtherValue>& b) const {
    assert(m_playlist_pimpl);
    shared_lock l(m_playlist_pimpl->mu);
    return std::distance(b.m_iterator, m_iterator);
}

} // namespace Core
} // namespace Melosic
