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

#include <thread>
#include <mutex>
namespace this_thread = std::this_thread;
using std::mutex;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <atomic>
#include <functional>

#include <boost/iostreams/read.hpp>
#include <boost/iostreams/compose.hpp>
#include <boost/iostreams/concepts.hpp>
namespace io = boost::iostreams;
#include <asio.hpp>
#include <boost/variant.hpp>

#include <melosic/common/error.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/common/range.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/common/smart_ptr_equality.hpp>
#include <melosic/asio/audio_io.hpp>
#include <melosic/melin/kernel.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/int_get.hpp>
#include <melosic/common/pcmbuffer.hpp>
#include <melosic/common/optional.hpp>
#include <melosic/melin/decoder.hpp>

#include "player.hpp"

namespace Melosic {
namespace Core {

struct Stopped;

struct NotifyPlayPos : Signals::Signal<Signals::Player::NotifyPlayPos> {
    using Super::Signal;
};

struct StateChanged : Signals::Signal<Signals::Player::StateChanged> {
    using Super::Signal;
};

struct State;

struct Player::impl : std::enable_shared_from_this<Player::impl> {
    explicit impl(Core::Kernel& kernel);

    ~impl();

    void play() {
        unique_lock l(mu);
        play_impl();
        if(state_impl() == DeviceState::Playing) {
            l.unlock();
            write_handler(std::error_code(), 0);
        }
    }
    void play_impl();

    void pause() {
        unique_lock l(mu);
        pause_impl();
    }
    void pause_impl();

    void stop() {
        unique_lock l(mu);
        stop_impl();
        for(auto& buf : in_buf)
            ::operator delete(ASIO::buffer_cast<void*>(buf));
        in_buf.clear();
    }
    void stop_impl();

    void seek(chrono::milliseconds dur);

    chrono::milliseconds tell() {
        unique_lock l(mu);
        return tell_impl();
    }
    chrono::milliseconds tell_impl();

    Output::DeviceState state() {
        unique_lock l(mu);
        return state_impl();
    }
    Output::DeviceState state_impl();

    void next() {
        unique_lock l(mu);
        next_impl();
    }
    void next_impl();

    void previous() {
        unique_lock l(mu);
        previous_impl();
    }
    void previous_impl();

    void jumpTo(int p) {
        unique_lock l(mu);
        jumpTo_impl(p);
    }
    void jumpTo_impl(int);

    void currentPlaylistChangedSlot(optional<Playlist>);
    void trackChangeSlot(int, optional<Track>);
    void changeDevice();
    void sinkChangeSlot();

    Core::Kernel& kernel;
    Melosic::Playlist::Manager& playman;
    Output::Manager& outman;
    Config::Manager& confman;

    mutex mu;

    StateChanged stateChanged;

    std::shared_ptr<State> m_current_state;
    friend struct State;

    NotifyPlayPos notifyPlayPosition;
    Logger::Logger logject{logging::keywords::channel = "Player"};
    friend class Player;

    template <typename S>
    auto changeState() -> decltype(m_current_state) {
        static_assert(std::is_base_of<State, S>::value, "Template parameter must be State derived");

        TRACE_LOG(logject) << "changeState(): changing state to " << typeid(S);

        return std::exchange(m_current_state, std::make_shared<S>(stateChanged));
    }

    std::unique_ptr<ASIO::AudioOutputBase> asioOutput;

    std::list<PCMBuffer> in_buf;

    void write_handler(std::error_code ec, std::size_t n);
    void read_handler(std::error_code ec, std::size_t n);

    optional<Playlist> m_current_playlist;
    Signals::ScopedConnection currentPlaylistConnection;
    std::unique_ptr<Decoder::PCMSource> m_current_source;
    std::unique_ptr<Decoder::PCMSource> m_next_source;
    Playlist::iterator m_current_iterator;
    chrono::milliseconds m_gapless_preload{1000};

    Config::Conf conf{"Player"};
    chrono::milliseconds buffer_time{1000};

    void loadedSlot(boost::synchronized_value<Config::Conf>& ubase) {
        TRACE_LOG(logject) << "Player conf loaded";

        auto base = ubase.synchronize();
        auto c = base->getChild("Player");
        if(!c) {
            base->putChild(conf);
            c = base->getChild("Player");
        }
        assert(c);
        c->merge(conf);
        c->addDefaultFunc([=]() { return conf; });
        c->iterateNodes([&] (const std::pair<Config::KeyType, Config::VarType>& pair) {
            TRACE_LOG(logject) << "Config: variable loaded: " << pair.first;
            variableUpdateSlot(pair.first, pair.second);
        });
        c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this);

        c = base->getChild("Output");
        if(!c)
            return;
        assert(c);
        c->iterateNodes([&] (const std::pair<Config::KeyType, Config::VarType>& pair) {
            variableUpdateSlot(pair.first, pair.second);
        });
        c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this);
    }

    void variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val) {
        using std::get;
        using Config::get;
        TRACE_LOG(logject) << "Config: variable updated: " << key;
        try {
            if(key == "buffer time")
                buffer_time = chrono::milliseconds(get<int64_t>(val));
            else if(key == "gapless preload time")
                m_gapless_preload = chrono::milliseconds(get<int64_t>(val));
        }
        catch(boost::bad_get&) {
            ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        }
    }

    static bool valid_iterator(Playlist::iterator it, Playlist& p) {
        return it != p.end();
    }
    static bool valid_iterator(Playlist::const_iterator it, const Playlist& p) {
        return it != p.end();
    }
};

static PCMBuffer widen(PCMBuffer in, const size_t n, const uint8_t bps_to) noexcept {
    assert(in.audio_specs.bps);
    assert(bps_to);
    assert(!(bps_to % 8));
    assert(!(in.audio_specs.bps % 8));
    assert(in.audio_specs.bps != bps_to);

    auto from = in.audio_specs.bps/8;
    auto to = bps_to/8;
    assert(!(n % from));

    PCMBuffer out{::operator new((n/from)*to), (n/from)*to};
    assert(ASIO::buffer_size(out) == (n/from)*to);

    auto out_ptr = ASIO::buffer_cast<char*>(out);

    auto in_ptr = ASIO::buffer_cast<const char*>(in);
    if(to > from) {
        for(auto i = 0u; i < n; i += from) {
            auto old_ptr = out_ptr;
            out_ptr = std::fill_n(out_ptr, to - from, 0);
            out_ptr = std::copy_n(std::next(in_ptr, i), from, out_ptr);
            assert(std::distance(old_ptr, out_ptr) == to);
        }
        assert(std::distance(ASIO::buffer_cast<char*>(out), out_ptr) == static_cast<ptrdiff_t>((n/from)*to));
    }
    else if(to < from) {
        assert(false);
        std::abort();
        for(auto i = 0u; i < n; i += from) {
            out_ptr = std::copy(in_ptr + i, in_ptr + i + to, out_ptr);
        }
    }
    else assert(false);

    return out;
}

struct State {
protected:
    explicit State(StateChanged& stateChanged) : stateChanged(stateChanged) {}

public:
    virtual ~State() {}

    //default actions (do nothing)
    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}

    virtual chrono::milliseconds tell() const {
        return 0ms;
    }

    virtual Output::DeviceState state() const = 0;

    virtual void sinkChange() {}

    static Player::impl* stateMachine;
    static Melosic::Playlist::Manager* playman;

    StateChanged& stateChanged;
};
decltype(State::stateMachine) State::stateMachine(nullptr);
decltype(State::playman) State::playman(nullptr);

void Player::impl::write_handler(std::error_code ec, std::size_t n) {
    unique_lock l(mu);
    if(state_impl() != Output::DeviceState::Playing)
        return;

    if(m_current_source) {
        notifyPlayPosition(m_current_source->tell(), m_current_source->duration());
        if(m_current_playlist && m_current_source->duration() - m_current_source->tell() < m_gapless_preload) {
            auto nt = m_current_iterator + 1;
            if(nt != m_current_playlist->end() && !m_next_source) {
                assert(nt != m_current_iterator);
                TRACE_LOG(logject) << "Pre-loading next track in list";
                m_next_source = kernel.getDecoderManager().open(*nt);
                assert(m_next_source);
            }
        }
    }
    TRACE_LOG(logject) << "write_handler: " << n << " bytes written";
    if(ec) {
        if(ec == std::errc::operation_canceled) {
            TRACE_LOG(logject) << ec.message();
            return;
        }
        ERROR_LOG(logject) << "write error: " << ec.message();
        stop_impl();
        return;
    }
    ec.clear();

    if(!m_current_playlist || m_current_iterator == m_current_playlist->end()) {
        TRACE_LOG(logject) << "no current track; stopping";
        ec = ASIO::error::make_error_code(ASIO::error::eof);
        n = 0;
    }
    else try {
        if(!m_current_source) {
            if(m_next_source) // get pre-loaded source if available
                m_current_source = std::move(m_next_source);
            else // otherwise open new source from playlist item. throws on error
                m_current_source = kernel.getDecoderManager().open(*m_current_iterator);
        }
        const auto as = m_current_source->getAudioSpecs();

        while(!in_buf.empty()) {
            if(ASIO::buffer_size(ASIO::mutable_buffer(in_buf.front())) <= 0)
                in_buf.pop_front();
            else {
                read_handler(ec, ASIO::buffer_size(ASIO::mutable_buffer(in_buf.front())));
                return;
            }
        }

        n = as.time_to_bytes(buffer_time);
        PCMBuffer tmp{::operator new(n), n};
        tmp.audio_specs = as;

        auto n_decoded = m_current_source->decode(tmp, ec);
        TRACE_LOG(logject) << "decoded to " << n_decoded << " bytes of PCM; " << tmp.audio_specs;

        if(n_decoded < n && !m_current_source->valid()) {
            TRACE_LOG(logject) << "track ended, starting next, if any";
            next_impl();
        }
        if(n_decoded == 0) {
            TRACE_LOG(logject) << "end of track";
            ::operator delete(ASIO::buffer_cast<void*>(tmp));
        }
        else if(as != asioOutput->current_specs()) {
            auto new_as = as;
            new_as.bps = asioOutput->current_specs().bps;
            if(new_as != asioOutput->current_specs()) {
                // stop + play to force device to re-prepare
                TRACE_LOG(logject) << "AudioSpecs mismatch";
                ::operator delete(ASIO::buffer_cast<void*>(tmp));
                n_decoded = 0;
                m_current_state->stop();
                m_current_state->play();
                l.unlock();
                write_handler(ec, 0);
                return;
            }
            else {
                // pad with zeroes or trim the fat
                TRACE_LOG(logject) << "bps mismatch; widening from " << (unsigned)as.bps
                                   << " to " << (unsigned)asioOutput->current_specs().bps;
                auto buf = widen(tmp, n_decoded, asioOutput->current_specs().bps);
                buf.audio_specs = tmp.audio_specs;
                buf.audio_specs.bps = asioOutput->current_specs().bps;
                n_decoded = ASIO::buffer_size(buf);
                in_buf.emplace_back(buf);

                ::operator delete(ASIO::buffer_cast<void*>(tmp));
            }
        }
        else
            in_buf.emplace_back(tmp);
        assert(!in_buf.empty());
        assert(in_buf.back().audio_specs == asioOutput->current_specs());
        n = n_decoded;
    }
    catch(...) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
        stop_impl();
        next_impl();
        if(m_current_iterator != m_current_playlist->end())
            play_impl();
    }
    read_handler(ec, n);
}

void Player::impl::read_handler(std::error_code ec, std::size_t n) {
    TRACE_LOG(logject) << "read_handler: " << n << " bytes read";
    if(ec || (in_buf.empty() && (ec = ASIO::error::make_error_code(ASIO::error::no_data)))) {
        if(ec.value() == ASIO::error::eof && m_current_source) {
            ec.clear();
            write_handler(ec, 0);
        }
        if(ec == std::errc::operation_canceled) {
            TRACE_LOG(logject) << ec.message();
            assert(in_buf.empty());
            return;
        }
        if(ec.value() == ASIO::error::eof) {
            TRACE_LOG(logject) << ec.message();
            assert(in_buf.empty());
            std::this_thread::sleep_for(buffer_time);
            stop_impl();
            return;
        }
        ERROR_LOG(logject) << "read error: " << ec.message();
        stop_impl();
        return;
    }
    assert(n > 0);
    assert(!in_buf.empty());

    PCMBuffer tmp{ASIO::buffer_cast<void*>(in_buf.front()), n};
    assert(ASIO::buffer_cast<void*>(in_buf.front()) != nullptr);
    in_buf.pop_front();
    assert(asioOutput);
    kernel.getThreadManager().enqueue([self=shared_from_this(),tmp]() mutable {
        unique_lock l(self->mu);
        ASIO::async_write(*self->asioOutput, ASIO::buffer(tmp),
            [tmp, self] (std::error_code ec, std::size_t n) {
                assert(ASIO::buffer_cast<void*>(tmp) != nullptr);
                if(n < ASIO::buffer_size(ASIO::mutable_buffer(tmp))) {
                    const auto s = ASIO::buffer_size(ASIO::mutable_buffer(tmp)) - n;
                    assert(s > 0);
                    auto* ptr = ::operator new(s);
                    std::memmove(ptr, ASIO::buffer_cast<uint8_t*>(tmp) + n, s);

                    self->in_buf.emplace_front(ptr, s);
                }
                ::operator delete(ASIO::buffer_cast<void*>(tmp));

                self->write_handler(ec, n);
            });
    });
}

struct Error;
struct Playing;

struct Stopped : State {
    explicit Stopped(StateChanged& sc) : State(sc) {
        stateChanged(Output::DeviceState::Stopped);
    }

    void play() override {
        assert(stateMachine->m_current_playlist);

        try {
            if(!stateMachine->valid_iterator(stateMachine->m_current_iterator, *stateMachine->m_current_playlist))
                stateMachine->jumpTo_impl(0);
            if(!stateMachine->valid_iterator(stateMachine->m_current_iterator, *stateMachine->m_current_playlist))
                return;
            assert(stateMachine->valid_iterator(stateMachine->m_current_iterator, *stateMachine->m_current_playlist));

            stateMachine->changeDevice();

            assert(stateMachine->asioOutput);
            if(!stateMachine->asioOutput)
                BOOST_THROW_EXCEPTION(std::exception());

            stateMachine->m_current_source = stateMachine->kernel.getDecoderManager()
                                                                 .open(*stateMachine->m_current_iterator);

            if(!stateMachine->m_current_source)
                return;
            const AudioSpecs as = stateMachine->m_current_source->getAudioSpecs();

            stateMachine->asioOutput->prepare(as);
            TRACE_LOG(stateMachine->logject) << "sink prepared with specs:\n" << as;
            stateMachine->asioOutput->play();
            auto ptr = stateMachine->changeState<Playing>();
            assert(ptr == this);
        }
        catch(...) {
            ERROR_LOG(stateMachine->logject) << boost::current_exception_diagnostic_information();
            stateMachine->changeState<Error>();
        }
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Stopped;
    }
};

struct Error : State {
    explicit Error(StateChanged& sc) : State(sc) {
        stateChanged(Output::DeviceState::Error);
    }

    void play() override {
        sinkChange();
        if(stateMachine->state_impl() != Output::DeviceState::Error)
            stateMachine->play_impl();
    }
    void pause() override {
        sinkChange();
        if(stateMachine->state_impl() != Output::DeviceState::Error) {
            stateMachine->play_impl();
            stateMachine->pause_impl();
        }
    }
    void stop() override {
        sinkChange();
        if(stateMachine->state_impl() != Output::DeviceState::Error)
            assert(stateMachine->state_impl() == Output::DeviceState::Stopped);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Error;
    }

    void sinkChange() override {
        try {
            stateMachine->changeDevice();
            if(stateMachine->asioOutput) {
                auto ptr = stateMachine->changeState<Stopped>();
                assert(ptr == this);
                assert(stateMachine->state_impl() == Output::DeviceState::Stopped);
            }
        }
        catch(...) {
            ERROR_LOG(stateMachine->logject) << boost::current_exception_diagnostic_information();
        }
    }
};

struct Stop : virtual State {
    explicit Stop(StateChanged& sc) : State(sc) {}
    virtual void stop() override {
        stateMachine->asioOutput->stop();
        auto ptr = stateMachine->changeState<Stopped>();
        assert(ptr == this);
    }
};

struct Tell : virtual State {
    explicit Tell(StateChanged& sc) : State(sc) {}
    virtual chrono::milliseconds tell() const override {
        if(!stateMachine->m_current_source)
            return 0ms;
        return stateMachine->m_current_source->tell();
    }
};

struct Paused;

struct Playing : virtual State, Stop, Tell {
    explicit Playing(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->asioOutput);
        assert(stateMachine->asioOutput->state() == Output::DeviceState::Playing);
        stateChanged(Output::DeviceState::Playing);
    }

    void pause() override {
        stateMachine->asioOutput->pause();
        auto ptr = stateMachine->changeState<Paused>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Playing;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->asioOutput->stop();
        stateMachine->changeDevice();
        stateMachine->play_impl();
        stateMachine->m_current_source->seek(time);
    }
};

struct Paused : virtual State, Stop, Tell {
    explicit Paused(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->asioOutput);
        stateChanged(Output::DeviceState::Paused);
    }

    void play() override {
        stateMachine->asioOutput->play();
        auto ptr = stateMachine->changeState<Playing>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Paused;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->asioOutput->stop();
        stateMachine->changeDevice();
        stateMachine->play_impl();
        stateMachine->m_current_source->seek(time);
        stateMachine->pause_impl();
    }
};

Player::impl::impl(Kernel& kernel) :
    kernel(kernel),
    playman(kernel.getPlaylistManager()),
    outman(kernel.getOutputManager()),
    confman(kernel.getConfigManager()),
    mu(),
    stateChanged(),
    m_current_state((State::stateMachine = this, //init statics before first construction
                   State::playman = &playman, //dirty comma operator usage
                   std::make_shared<Stopped>(stateChanged))),
    logject(logging::keywords::channel = "StateMachine")
{
    conf.putNode("gapless preload time", static_cast<int64_t>(m_gapless_preload.count()));
    playman.getCurrentPlaylistChangedSignal().connect(&impl::currentPlaylistChangedSlot, this);
    outman.getPlayerSinkChangedSignal().connect(&impl::sinkChangeSlot, this);
    confman.getLoadedSignal().connect(&impl::loadedSlot, this);
    notifyPlayPosition.connect([this] (chrono::milliseconds pos, chrono::milliseconds dur) {
        TRACE_LOG(logject) << "pos: " << pos.count() << "; dur: " << dur.count();
    });
}

Player::impl::~impl() {
    stop();
    lock_guard l(mu);
    if(asioOutput)
        asioOutput->cancel();
    asioOutput.reset();
}

void Player::impl::play_impl() {
    m_current_state->play();
}

void Player::impl::pause_impl() {
    m_current_state->pause();
}

void Player::impl::stop_impl() {
    m_current_state->stop();
    if(m_current_source) {
        m_current_source->reset();
        m_current_source.reset();
    }
    jumpTo_impl(m_current_playlist->size());
}

void Player::impl::seek(chrono::milliseconds dur) {
    TRACE_LOG(logject) << "Seek...";
    if(m_current_source)
        m_current_source->seek(dur);
}

chrono::milliseconds Player::impl::tell_impl() {
    return m_current_state->tell();
}

Output::DeviceState Player::impl::state_impl() {
    return m_current_state->state();
}

void Player::impl::next_impl() {
    if(!m_current_playlist) {
        m_current_source.reset();
        m_current_iterator = {};
        return;
    }
    if(m_current_source) {
        m_current_source->reset();
        m_current_source.reset();
    }
    if(m_current_iterator != m_current_playlist->end()) {
        ++m_current_iterator;
        if(m_next_source)
            m_current_source = std::move(m_next_source);
        else if(m_current_iterator != m_current_playlist->end())
            m_current_source = kernel.getDecoderManager().open(*m_current_iterator);
    }
    if(m_current_iterator == m_current_playlist->end())
        m_current_iterator = m_current_playlist->end();
}

void Player::impl::previous_impl() {
    if(!m_current_playlist) {
        m_current_source.reset();
        m_next_source.reset();
        m_current_iterator = {};
        return;
    }
    m_next_source.reset();
    if(m_current_iterator != m_current_playlist->begin()) {
        if(m_current_source) {
            m_current_source->reset();
            m_current_source.reset();
        }
        --m_current_iterator;
    }
    else if(m_current_source)
        m_current_source->reset();
}

void Player::impl::jumpTo_impl(int p) {
    if(!m_current_playlist) {
        m_current_source.reset();
        m_next_source.reset();
        m_current_iterator = {};
        return;
    }
    if(m_current_source) {
        m_current_source->reset();
        m_current_source.reset();
    }
    if(p < m_current_playlist->size() && p >= 0)
        m_current_iterator = m_current_playlist->begin() + p;
    else
        m_current_iterator = m_current_playlist->end();
    if(p == 1)
        m_current_source = std::move(m_next_source);
}

void Player::impl::currentPlaylistChangedSlot(optional<Playlist> p) {
    m_current_playlist = p;
    jumpTo(0);
}

void Player::impl::trackChangeSlot(int, optional<Track> t) {
//    if(!m_current_track)
//        m_current_track = t;
    assert(false);
}

void Player::impl::changeDevice() {
    TRACE_LOG(logject) << "changeDevice()";
    if(asioOutput) {
        asioOutput->cancel();
        asioOutput->stop();
    }
    asioOutput = std::move(outman.createASIOSink());
}

void Player::impl::sinkChangeSlot() {
    TRACE_LOG(logject) << "sinkChangeSlot()";
    unique_lock l(mu);
    try {
        m_current_state->sinkChange();
    }
    catch(...) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();

        stop_impl();
        changeState<Error>();
    }
}

Player::Player(Kernel& kernel) :
    pimpl(new impl(kernel))
{}

Player::~Player() {
    pimpl->stop();
}

void Player::play() {
    pimpl->play();
}

void Player::pause() {
    pimpl->pause();
}

void Player::stop() {
    pimpl->stop();
}

DeviceState Player::state() const {
    return pimpl->state();
}

void Player::seek(chrono::milliseconds dur) {
    pimpl->seek(dur);
}

chrono::milliseconds Player::tell() const {
    return pimpl->tell();
}

optional<Playlist> Player::currentPlaylist() const {
    lock_guard l(pimpl->mu);
    return pimpl->m_current_playlist;
}

optional<Track> Player::currentTrack() const {
    lock_guard l(pimpl->mu);
    return nullopt;
//    return pimpl->m_current_track;
}

void Player::next() {
    pimpl->next();
}

void Player::previous() {
    pimpl->previous();
}

void Player::jumpTo(int p) {
    pimpl->jumpTo(p);
}

tuple<optional<Playlist>, optional<Track> > Player::current() const {
    lock_guard l(pimpl->mu);
    return std::make_tuple(pimpl->m_current_playlist, nullopt);
}

Signals::Player::StateChanged& Player::stateChangedSignal() const {
    return pimpl->stateChanged;
}

} // namespace Core
} // namespace Melosic

