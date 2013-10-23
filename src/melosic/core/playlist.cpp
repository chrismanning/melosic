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

#include <boost/thread/shared_mutex.hpp>
using mutex = boost::shared_mutex;
using lock_guard = std::lock_guard<mutex>;
using unique_lock = std::unique_lock<mutex>;
using shared_lock = boost::shared_lock<mutex>;
#include <boost/format.hpp>
using boost::format;
#include <boost/scope_exit.hpp>
#include <boost/iostreams/read.hpp>
namespace io = boost::iostreams;
#include <boost/range/adaptor/sliced.hpp>

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

struct CurrentTrackChanged : Signals::Signal<Signals::Playlist::CurrentTrackChanged> {
    using Super::Signal;
};

static CurrentTrackChanged currentTrackChangedSignal;

static void currentTrackChanged(int i, std::optional<Core::Track> t, unique_lock& l) {
    l.unlock();
    BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
    currentTrackChangedSignal(i, t);
}

class Playlist::impl {
public:
    impl(Decoder::Manager& decman, std::string name) :
        decman(decman),
        name(name)
    {}

    //IO
    std::streamsize read(char* s, std::streamsize n, unique_lock& l) {
        if(!m_current_track)
            return -1;

        auto r = io::read(*m_current_track, s, n);
        if(r < n && !*m_current_track) {
            TRACE_LOG(logject) << "tell(): " << m_current_track->tell().count();
            TRACE_LOG(logject) << "duration(): " << m_current_track->duration().count();
            const AudioSpecs& as = m_current_track->getAudioSpecs();
            m_current_track->close();
            next(l);
            if(m_current_track) {
                if(m_current_track->getAudioSpecs() != as)
                    return r;
                if(r < 0)
                    r = 0;
                m_current_track->reOpen();
                m_current_track->reset();
                r += io::read(*m_current_track, s + r, n - r);
            }
        }
        return r;
    }

    void trackAdded(int i, std::optional<Core::Track> t, unique_lock& l) {
        if(i < 0 || !t)
            return;
        t->getTagsChangedSignal().connect([it=std::next(std::begin(tracks), i), this]
                                          (const TagLib::PropertyMap& tags) mutable {
            tagsChangedSignal(+std::distance(std::begin(tracks), it), std::cref(tags));
        });
        l.unlock();
        BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
        trackAddedSignal(i, t);
    }

    void trackRemoved(int i, std::optional<Core::Track> t, unique_lock& l) {
        l.unlock();
        BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
        trackRemovedSignal(i, t);
    }

    void tagsChanged(int i, const TagLib::PropertyMap& tags, unique_lock& l) {
        l.unlock();
        BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
        tagsChangedSignal(i, std::cref(tags));
    }

    void seek(chrono::milliseconds dur) {
        if(m_current_track)
            m_current_track->seek(dur);
    }

    //playlist controls
    chrono::milliseconds duration() {
        return std::accumulate(std::begin(tracks), std::end(tracks), 0ms,
                               [](chrono::milliseconds a, Track& b) { return a + b.duration(); });
    }

    void previous(unique_lock& l) {
        if(!m_current_track)
            return;
        if(m_current_pos == 0) {
            jumpTo(m_current_pos, l);
            seek(0ms);
        }
        m_current_track->reset();
        jumpTo(--m_current_pos, l);
    }

    void next(unique_lock& l) {
        if(!m_current_track)
            return;
        m_current_track->reset();
        jumpTo(++m_current_pos, l);
    }

    void jumpTo(Playlist::size_type pos, unique_lock& l) {
        if(m_current_track)
            m_current_track->reset();

        if(pos >= size() || pos < 0) {
            m_current_pos = -1;
            m_current_track = std::nullopt;
        }
        else {
            m_current_pos = pos;
            m_current_track = *std::next(std::begin(tracks), m_current_pos);
        }
        currentTrackChanged(m_current_pos, m_current_track, l);
    }

    auto currentTrack() {
        return m_current_track;
    }

    //capacity
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

        if(size() == 1)
            jumpTo(0, l);
        return *r;
    }

    std::optional<Core::Track> emplace(Playlist::size_type pos, boost::filesystem::path filename,
                                       chrono::milliseconds start, chrono::milliseconds end,
                                       unique_lock& l)
    {
        if(pos > size())
            pos = size();
        auto v = decman.openTrack(std::move(filename), start, end);
        if(!v)
            return {};
        auto r = tracks.emplace(std::next(std::begin(tracks), pos), *v);

        trackAdded(pos, *r, l);

        if(size() == 1)
            jumpTo(0, l);

        return *r;
    }

    Playlist::size_type emplace(Playlist::size_type pos,
                                ForwardRange<const boost::filesystem::path> values,
                                unique_lock& l)
    {
        if(pos > size())
            pos = size();

        Playlist::size_type s{0};

        if(values.empty())
            return s;

        for(const auto& path : values) {
            try {
                auto v = decman.openTrack(path);
                if(!v)
                    continue;
                tracks.emplace(std::next(std::begin(tracks), pos+s), *v);

                trackAdded(pos, v, l);

                s++;
            }
            catch(...) {
                ERROR_LOG(logject) << format("Track couldn't be added to playlist \"%1%\" %2%:%3%")
                                      % name % __FILE__ % __LINE__;
                ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
            }
        }

        if(size() == 1)
            jumpTo(0, l);

        return s;
    }

    void push_back(Playlist::value_type value, unique_lock& l) {
        tracks.push_back(std::move(value));

        trackAdded(tracks.size()-1, tracks.back(), l);

        if(size() == 1)
            jumpTo(0, l);
    }

    void emplace_back(boost::filesystem::path filename,
                      chrono::milliseconds start,
                      chrono::milliseconds end,
                      unique_lock& l)
    {
        auto v = decman.openTrack(std::move(filename), start, end);
        if(!v)
            return;
        tracks.emplace_back(*v);

        trackAdded(tracks.size()-1, tracks.back(), l);

        if(size() == 1)
            jumpTo(0, l);
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
    Playlist::list_type tracks;
    std::optional<Playlist::value_type> m_current_track;
    Playlist::size_type m_current_pos{-1};
    TrackAdded trackAddedSignal;
    TrackRemoved trackRemovedSignal;
    TagsChanged tagsChangedSignal;
    MultiTagsChanged multiTagsChangedSignal;

    Decoder::Manager& decman;
    std::string name;
};

Playlist::Playlist(Decoder::Manager& decman, std::string name) :
    pimpl(new impl(decman, std::move(name))) {}

Playlist::~Playlist() {}

//IO
std::streamsize Playlist::read(char* s, std::streamsize n) {
    unique_lock l(pimpl->mu);
    return pimpl->read(s, n, l);
}

void Playlist::seek(chrono::milliseconds dur) {
    lock_guard l(pimpl->mu);
    pimpl->seek(dur);
}

//playlist controls
chrono::milliseconds Playlist::duration() const {
    shared_lock l(pimpl->mu);
    return pimpl->duration();
}

void Playlist::previous() {
    unique_lock l(pimpl->mu);
    pimpl->previous(l);
}

void Playlist::next() {
    unique_lock l(pimpl->mu);
    pimpl->next(l);
}

void Playlist::jumpTo(Playlist::size_type pos) {
    unique_lock l(pimpl->mu);
    pimpl->jumpTo(pos, l);
}

std::optional<Playlist::value_type> Playlist::currentTrack() {
    shared_lock l(pimpl->mu);
    return pimpl->currentTrack();
}

const std::optional<Playlist::value_type> Playlist::currentTrack() const {
    shared_lock l(pimpl->mu);
    return pimpl->currentTrack();
}

std::optional<Playlist::value_type> Playlist::getTrack(size_type pos) {
    shared_lock l(pimpl->mu);
    if(pos >= pimpl->size())
        return {};
    return {pimpl->tracks[pos]};
}

const std::optional<Playlist::value_type> Playlist::getTrack(size_type pos) const {
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
            t.reOpen();
            t.close();
        }
        catch(...) {}
    }
    l.unlock();
    pimpl->multiTagsChangedSignal(start, end);
}

//capacity
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

Playlist::operator bool() const {
    shared_lock l(pimpl->mu);
    return pimpl->empty() ? false : static_cast<bool>(pimpl->currentTrack());
}

std::optional<Playlist::value_type> Playlist::insert(size_type pos, Playlist::value_type value) {
    unique_lock l(pimpl->mu);
    return pimpl->insert(pos, std::move(value), l);
}

std::optional<Playlist::value_type> Playlist::emplace(size_type pos,
                                                      boost::filesystem::path filename,
                                                      chrono::milliseconds start,
                                                      chrono::milliseconds end)
{
    unique_lock l(pimpl->mu);
    return pimpl->emplace(pos, std::move(filename), start, end, l);
}

Playlist::size_type Playlist::insert(size_type pos, ForwardRange<value_type> values) {
    unique_lock l(pimpl->mu);
    size_type s{0};
    for(auto v : values) {
        pimpl->insert(pos++, v, l);
        ++s;
    }
    return s;
}

Playlist::size_type Playlist::emplace(size_type pos, ForwardRange<const boost::filesystem::path> values) {
    unique_lock l(pimpl->mu);
    return pimpl->emplace(pos, values, l);
}

void Playlist::push_back(Playlist::value_type value) {
    unique_lock l(pimpl->mu);
    pimpl->push_back(std::move(value), l);
}

void Playlist::emplace_back(boost::filesystem::path filename,
                            chrono::milliseconds start,
                            chrono::milliseconds end)
{
    unique_lock l(pimpl->mu);
    pimpl->emplace_back(std::move(filename), start, end, l);
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

void Playlist::swap(Playlist& b)  {
    lock_guard l1(pimpl->mu);
    lock_guard l2(b.pimpl->mu);
    std::swap(pimpl, b.pimpl);
}

const std::string& Playlist::getName() const {
    shared_lock l(pimpl->mu);
    return pimpl->getName();
}

void Playlist::setName(std::string name) {
    lock_guard l(pimpl->mu);
    pimpl->setName(std::move(name));
}

bool Playlist::operator==(const Playlist& b) const {
    return pimpl == b.pimpl;
}

Signals::Playlist::TagsChanged& Playlist::getTagsChangedSignal() const noexcept {
    return pimpl->tagsChangedSignal;
}

Signals::Playlist::MultiTagsChanged& Playlist::getMutlipleTagsChangedSignal() const noexcept {
    return pimpl->multiTagsChangedSignal;
}

Signals::Playlist::CurrentTrackChanged& Playlist::getCurrentTrackChangedSignal() noexcept {
    return currentTrackChangedSignal;
}

} // namespace Core
} // namespace Melosic
