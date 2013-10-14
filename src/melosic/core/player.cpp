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

#include <array>
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
namespace io = boost::iostreams;
#include <boost/scope_exit.hpp>
#include <boost/asio.hpp>
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
            const AudioSpecs& as = playman.currentPlaylist()->currentTrack()->getAudioSpecs();
            write_handler(boost::system::error_code(),
                          (as.target_bps/8) * (as.target_sample_rate/1000.0) * buffer_time.count());
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

    void trackChangeSlot(int, std::optional<Track>);
    void changeDevice();
    void sinkChangeSlot();

    Core::Kernel& kernel;
    Melosic::Playlist::Manager& playman;
    Output::Manager& outman;
    Config::Manager& confman;

    mutex mu;

    StateChanged stateChanged;

    std::shared_ptr<State> currentState_;
    friend struct State;

    NotifyPlayPos notifyPlayPosition;
    Logger::Logger logject{logging::keywords::channel = "Player"};
    friend class Player;

    template <typename S>
    auto changeState() -> decltype(currentState_) {
        static_assert(std::is_base_of<State, S>::value, "Template parameter must be State derived");

        TRACE_LOG(logject) << "changeState(): changing state to " << typeid(S);

        auto r(currentState_);
        assert(r);
        currentState_.reset(new S(stateChanged));
        assert(currentState_);
        return std::move(r);
    }

    std::unique_ptr<ASIO::AudioOutputBase> asioOutput;

    std::list<ASIO::mutable_buffer> in_buf;

    struct Widener : io::multichar_input_filter {
        struct category : io::multichar_input_filter::category, io::optimally_buffered_tag {};

        Widener(std::size_t buffer_size, uint16_t from, uint16_t to) noexcept :
            buffer_size(buffer_size),
            from(from),
            to(to)
        {
            assert(from);
            assert(to);
            assert(!(to % 8));
            assert(!(from % 8));
        }

        template<typename Source>
        std::streamsize read(Source& src, char* s, std::streamsize n) {
            if(to == from)
                return io::read(src, s, n);

            assert(!(n % (to/8)));
            const auto tbytes = (to/8);
            const auto fbytes = (from/8);

            std::streamsize i{0}, r;
            if(to > from)
                for(i = 0; i < n; i += tbytes) {
                    for(int j = 0; j < (tbytes - fbytes); j++)
                        *s++ = 0;
                    r = io::read(src, s, fbytes);
                    if(r <= 0)
                        return -1;
                    s += r;
                }
            else if(to < from)
                for(i = 0; i < n; i += tbytes) {
                    char c[fbytes - tbytes];
                    r = io::read(src, c, fbytes - tbytes);
                    assert(r == fbytes - tbytes);
                    r = io::read(src, s, tbytes);
                    if(r <= 0)
                        return -1;
                    s += tbytes;
                }
            else assert(false);

            return i;
        }

        std::streamsize optimal_buffer_size() const noexcept {
            return buffer_size;
        }

    private:
        const std::size_t buffer_size;
        const uint16_t from, to;
    };

    void write_handler(boost::system::error_code ec, std::size_t n) {
        unique_lock l(mu);
        TRACE_LOG(logject) << "write_handler: " << n << " bytes written";
        if(ec) {
            if(ec.value() == boost::system::errc::operation_canceled) {
                TRACE_LOG(logject) << "write error: " << ec.message();
                return;
            }
            ERROR_LOG(logject) << "write error: " << ec.message();
            stop_impl();
            return;
        }
        ec.clear();

        auto playlist = playman.currentPlaylist();

        if(!playlist) {
            ec = ASIO::error::make_error_code(ASIO::error::no_data);
            n = 0;
            stop_impl();
        }
        else if(!*playlist) {
            ec = ASIO::error::make_error_code(ASIO::error::eof);
            n = 0;
            stop_impl();
            playlist->jumpTo(0);
        }
        else try {
            const AudioSpecs& as = playlist->currentTrack()->getAudioSpecs();

            n = (as.target_bps/8) * (as.target_sample_rate/1000.0) * buffer_time.count();
            ASIO::mutable_buffer tmp{::operator new(n), n};
            in_buf.emplace_back(tmp);

            Widener widen{n, as.bps, as.target_bps};
            TRACE_LOG(logject) << "widening from " << (unsigned)as.bps << " to " << (unsigned)as.target_bps;

            auto composite = io::compose(widen, boost::ref(*playlist));

            auto n_ = io::read(composite, ASIO::buffer_cast<char*>(tmp), n);
            if(n_ <= 0) {
                ec = ASIO::error::make_error_code(ASIO::error::eof);
                n_ = 0;
                stop_impl();
                playlist->jumpTo(0);
            }
            n = static_cast<std::size_t>(n_);
        }
        catch(...) {
            ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
            stop_impl();
            playlist->next();
            play_impl();
        }
        read_handler(ec, n);
    }

    void read_handler(boost::system::error_code ec, std::size_t n) {
        TRACE_LOG(logject) << "read_handler: " << n << " bytes read";
        if(ec) {
            if(ec.value() == boost::system::errc::operation_canceled || ec.value() == ASIO::error::eof) {
                TRACE_LOG(logject) << "read error: " << ec.message();
                return;
            }
            ERROR_LOG(logject) << "read error: " << ec.message();
            stop_impl();
            return;
        }
        if(in_buf.empty()) {
            ERROR_LOG(logject) << "buffer is empty, abandon ship";
            stop_impl();
            return;
        }

        ASIO::mutable_buffer tmp{in_buf.front()};
        in_buf.pop_front();
        assert(asioOutput);
        kernel.getThreadManager().enqueue([self=shared_from_this(),this,tmp]() {
            unique_lock l(mu);
            ASIO::async_write(*asioOutput, ASIO::buffer(tmp),
                [tmp, self] (boost::system::error_code ec, std::size_t n) {
                    if(n < ASIO::buffer_size(tmp)) {
                        ASIO::mutable_buffer tmp2;
                        ASIO::buffer_copy(tmp2, tmp + n);
                        self->in_buf.push_front(tmp2);
                    }
                    ::operator delete(ASIO::buffer_cast<void*>(tmp));

                    self->write_handler(ec, n);
                });
        });
    }

    Config::Conf conf{"Player"};
    chrono::milliseconds buffer_time{1000};

    void loadedSlot(Config::Conf& base) {
        TRACE_LOG(logject) << "Player conf loaded";

        auto c = base.getChild("Player");
        if(!c) {
            base.putChild(conf);
            c = base.getChild("Player");
        }
        assert(c);
        c->merge(conf);
        c->addDefaultFunc([=]() -> Config::Conf { return conf; });
        c->iterateNodes([&] (const std::pair<Config::KeyType, Config::VarType>& pair) {
            TRACE_LOG(logject) << "Config: variable loaded: " << pair.first;
            variableUpdateSlot(pair.first, pair.second);
        });
        c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this);

        c = base.getChild("Output");
        if(!c)
            return;
        assert(c);
        c->iterateNodes([&] (const std::pair<Config::KeyType, Config::VarType>& pair) {
            variableUpdateSlot(pair.first, pair.second);
        });
        c->getVariableUpdatedSignal().connect(&impl::variableUpdateSlot, this);
    }

    void variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val) {
        using boost::get;
        using Config::get;
        TRACE_LOG(logject) << "Config: variable updated: " << key;
        try {
            if(key == "buffer time") {
                buffer_time = chrono::milliseconds(get<size_t>(val));
            }
        }
        catch(boost::bad_get&) {
            ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        }
    }
};

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

struct Error;
struct Playing;

struct Stopped : State {
    explicit Stopped(StateChanged& sc) : State(sc) {
        stateChanged(Output::DeviceState::Stopped);
    }

    void play() override {
        assert(playman->currentPlaylist());

        try {
            auto ct = playman->currentPlaylist()->currentTrack();
            if(!ct) {
                playman->currentPlaylist()->jumpTo(0);
                ct = playman->currentPlaylist()->currentTrack();
            }
            if(!ct)
                return;

            stateMachine->changeDevice();
            if(!stateMachine->asioOutput)
                BOOST_THROW_EXCEPTION(std::exception());

            auto& as = ct->getAudioSpecs();
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
        assert(playman->currentPlaylist());
        auto playlist = playman->currentPlaylist();
        assert(playlist);
        if(!*playlist)
            return 0ms;
        return playlist->currentTrack()->tell();
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
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
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
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
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
    currentState_((State::stateMachine = this, //init statics before first construction
                   State::playman = &playman, //dirty comma operator usage
                   std::make_shared<Stopped>(stateChanged))),
    logject(logging::keywords::channel = "StateMachine")
{
    playman.getTrackChangedSignal().connect(&impl::trackChangeSlot, this);
    outman.getPlayerSinkChangedSignal().connect(&impl::sinkChangeSlot, this);
    confman.getLoadedSignal().connect(&impl::loadedSlot, this);
}

Player::impl::~impl() {
    stop();
    lock_guard l(mu);
    if(asioOutput)
        asioOutput->cancel();
    asioOutput.reset();
}

void Player::impl::play_impl() {
    currentState_->play();
}

void Player::impl::pause_impl() {
    currentState_->pause();
}

void Player::impl::stop_impl() {
    currentState_->stop();
    if(playman.currentPlaylist() && *playman.currentPlaylist()) {
        playman.currentPlaylist()->currentTrack()->reset();
        playman.currentPlaylist()->currentTrack()->close();
    }
}

void Player::impl::seek(chrono::milliseconds dur) {
    TRACE_LOG(logject) << "Seek...";
    if(playman.currentPlaylist())
        playman.currentPlaylist()->seek(dur);
}

chrono::milliseconds Player::impl::tell_impl() {
    return currentState_->tell();
}

Output::DeviceState Player::impl::state_impl() {
    return currentState_->state();
}

void Player::impl::trackChangeSlot(int, std::optional<Track> track) {
    if(!track)
        return;
    auto cp = playman.currentPlaylist();
    assert(cp);
    try {
        if(*cp && asioOutput) {
            assert(track == *cp->currentTrack());
            if(asioOutput->currentSpecs() != track->getAudioSpecs()) {
                LOG(logject) << "Changing sink specs for new track";
                TRACE_LOG(logject) << "new track specs: " << track->getAudioSpecs();
                asioOutput->stop();
                this_thread::sleep_for(100ms);
                asioOutput->prepare(track->getAudioSpecs());
                TRACE_LOG(logject) << "prepared track specs: " << track->getAudioSpecs();
                this_thread::sleep_for(100ms);
                asioOutput->play();
            }
            else {
                auto& as1 = track->getAudioSpecs();
                auto& as2 = asioOutput->currentSpecs();
                as1.target_bps = as2.target_bps;
                as1.target_sample_rate = as2.target_sample_rate;
            }
        }
    }
    catch(DeviceException& e) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
        changeState<Error>();
    }
    catch(...) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
        playman.currentPlaylist()->next();
    }
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
        currentState_->sinkChange();
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

Signals::Player::StateChanged& Player::stateChangedSignal() const {
    return pimpl->stateChanged;
}

} // namespace Core
} // namespace Melosic

