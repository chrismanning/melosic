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
using std::thread; using std::mutex;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <atomic>
#include <functional>

#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>
namespace io = boost::iostreams;
#include <boost/scope_exit.hpp>
#include <boost/asio.hpp>
#include <boost/asio/use_future.hpp>

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

struct Player::impl {
    explicit impl(Melosic::Playlist::Manager& playman, Output::Manager& outman);

    ~impl();

    void play() {
        unique_lock l(mu);
        play_impl();
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

    void trackChangeSlot(const Track& track);
    void changeDevice();
    void sinkChangeSlot();

    Melosic::Playlist::Manager& playman;

    Output::Manager& outman;
    mutex mu;

    StateChanged stateChanged;

    std::shared_ptr<State> currentState_;
    friend struct State;

    NotifyPlayPos notifyPlayPosition;
    std::atomic_bool end;
    thread playerThread;
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
private:
    void async_loop() {
        std::shared_ptr<Playlist> playlist;
        while(!end) {
            if(state_impl() != DeviceState::Playing || !asioOutput) {
                this_thread::sleep_for(50ms);
                continue;
            }
            unique_lock l(mu);
            try {
                BOOST_SCOPE_EXIT_ALL(&) {
                    if(static_cast<bool>(*playlist)) {
                        l.unlock();
                        BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
                        notifyPlayPosition(playlist->currentTrack()->tell(),
                                           playlist->currentTrack()->duration());
                    }
                };

                TRACE_LOG(logject) << "In play loop";
                playlist = playman.currentPlaylist();

                //TODO: 3200 is temp value
                int8_t sbuf[3200];
                auto a = io::read(*playlist, (char*)sbuf, sizeof(sbuf));

                if(a == -1) {//end-of-playlist
                    stop_impl();
                    l.unlock();
                    BOOST_SCOPE_EXIT_ALL(&l) { l.lock(); };
                    playlist->jumpTo(0);
                    continue;
                }
                boost::system::error_code ec;
//                auto r = ASIO::write(*asioOutput, ASIO::buffer(sbuf, a), ec);

                auto f = ASIO::async_write(*asioOutput, ASIO::buffer(sbuf, a), ASIO::use_future);
                f.wait();
                if(ec)
                    ERROR_LOG(logject) << ec.message();
//                else
//                    assert(r == static_cast<size_t>(a));
            }
            catch(...) {
                ERROR_LOG(logject) << boost::current_exception_diagnostic_information();
                stop_impl();
                l.unlock();
                BOOST_SCOPE_EXIT_ALL(&l) { if(!l.owns_lock()) l.lock(); };
                playman.currentPlaylist()->next();
                l.lock();
                play_impl();
            }
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
        if(!*playman->currentPlaylist())
            return;

        try {
            stateMachine->changeDevice();
            if(!stateMachine->asioOutput)
                BOOST_THROW_EXCEPTION(std::exception());
            stateMachine->asioOutput->prepare(playman->currentPlaylist()->currentTrack()->getAudioSpecs());
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

Player::impl::impl(Melosic::Playlist::Manager& playman, Output::Manager& outman) :
    playman(playman),
    outman(outman),
    mu(),
    stateChanged(),
    currentState_((State::stateMachine = this, //init statics before first construction
                   State::playman = &playman, //dirty comma operator usage
                   std::make_shared<Stopped>(stateChanged))),
    end(false),
    playerThread(&impl::async_loop, this),
    logject(logging::keywords::channel = "StateMachine")
{
    playman.getTrackChangedSignal().connect(&impl::trackChangeSlot, this);
    outman.getPlayerSinkChangedSignal().connect(&impl::sinkChangeSlot, this);
}

Player::impl::~impl() {
    end = true;
    if(playerThread.joinable()) {
        TRACE_LOG(logject) << "Closing thread";
        playerThread.join();
    }
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
    if(playman.currentPlaylist()) {
        playman.currentPlaylist()->seek(dur);
    }
}

chrono::milliseconds Player::impl::tell_impl() {
    return currentState_->tell();
}

Output::DeviceState Player::impl::state_impl() {
    return currentState_->state();
}

void Player::impl::trackChangeSlot(const Track& track) {
    auto cp = playman.currentPlaylist();
    assert(cp);
    try {
        if(*cp && asioOutput) {
            assert(track == *cp->currentTrack());
            if(asioOutput->currentSpecs() != track.getAudioSpecs()) {
                LOG(logject) << "Changing sink specs for new track";
                asioOutput->stop();
                this_thread::sleep_for(100ms);
                asioOutput->prepare(const_cast<Track&>(track).getAudioSpecs());
                this_thread::sleep_for(100ms);
                asioOutput->play();
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
//    lock_guard l(sinkWrapper_.mu);
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

Player::Player(Melosic::Playlist::Manager& playlistman, Output::Manager& outman) :
    pimpl(new impl(playlistman, outman))
{}

Player::~Player() {}

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

