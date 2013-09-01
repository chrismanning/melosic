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

    void play();
    void pause();
    void stop();
    void seek(chrono::milliseconds dur);
    chrono::milliseconds tell();
    Output::DeviceState state();
    Output::PlayerSink& sink();
    void trackChangeSlot(const Track& track);
    void changeDevice();
    void sinkChangeSlot();

    Melosic::Playlist::Manager& playman;
    struct SinkWrapper : Output::PlayerSink {
        std::streamsize write(const char* s, std::streamsize n) override {
            lock_guard l(mu);
            assert(device);
            return device->write(s, n);
        }
        const std::string& getSinkName() override {
            lock_guard l(mu);
            assert(device);
            return device->getSinkName();
        }
        void prepareSink(Melosic::AudioSpecs& as) override {
            lock_guard l(mu);
            assert(device);
            device->prepareSink(as);
        }
        const Melosic::AudioSpecs& currentSpecs() override {
            lock_guard l(mu);
            assert(device);
            return device->currentSpecs();
        }
        const std::string& getSinkDescription() override {
            lock_guard l(mu);
            assert(device);
            return device->getSinkDescription();
        }
        void play() override {
            lock_guard l(mu);
            assert(device);
            device->play();
        }
        void pause() override {
            lock_guard l(mu);
            assert(device);
            device->pause();
        }
        void stop() override {
            lock_guard l(mu);
            assert(device);
            device->stop();
        }
        Output::DeviceState state() override {
            lock_guard l(mu);
            assert(device);
            return device->state();
        }

        explicit operator bool() {
            lock_guard l(mu);
            return static_cast<bool>(device);
        }

        mutex mu;

        std::unique_ptr<PlayerSink> device;
    } sinkWrapper_;

    Output::Manager& outman;
    mutex mu;

    std::function<void()> restoreFun;

    std::shared_ptr<State> currentState_;
    friend struct State;

    StateChanged stateChanged;

    NotifyPlayPos notifyPlayPosition;
    std::atomic_bool end;
    thread playerThread;
    Logger::Logger logject{logging::keywords::channel = "Player"};
    friend class Player;

    template <typename S>
    auto changeState() -> decltype(currentState_) {
        static_assert(std::is_base_of<State, S>::value, "Template parameter must be State derived");

        TRACE_LOG(logject) << "changeState(): changing state to " << typeid(S);
        lock_guard l(mu);

        auto r(currentState_);
        assert(r);
        currentState_.reset(new S(stateChanged));
        assert(currentState_);
        return std::move(r);
    }

private:
    void start() {
        TRACE_LOG(logject) << "Thread starting";
        std::array<uint8_t,16384> s;
        std::streamsize n = 0;
        while(!end) {
            std::shared_ptr<Playlist> playlist;

            this_thread::sleep_for(10ms);
            try {
                playlist = playman.currentPlaylist();
                if(!playlist) {
                    ERROR_LOG(logject) << "No playlist";
                    continue;
                }
                switch(state()) {
                    case DeviceState::Playing: {
//                            std::clog << "play pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                        if(n < 1024) {
                            auto a = io::read(*playlist, (char*)s.data(), s.size());
                            if(static_cast<bool>(*playlist))
                                notifyPlayPosition(playlist->currentTrack()->tell(),
                                                   playlist->currentTrack()->duration());

                            if(a == -1) {
                                if(n == 0) {
                                    stop();
                                    playlist->currentTrack() = playlist->begin();
                                }
                                break;
                            }
                            else
                                n += a;
                        }
                        TRACE_LOG(logject) << "playing... " << tell().count();
                        n -= io::write(sink(), (char*)s.data()+s.size()-n, n);
                        break;
                    }
                    case DeviceState::Error:
//                            ERROR_LOG(logject) << "Irrecoverable error occured";
//                            currentState()->device().reset();
                        break;
                    case DeviceState::Stopped:
                        n = 0;
                    case DeviceState::Paused:
//                            std::clog << "paused pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                    case DeviceState::Ready:
                    default:
                        break;
                }
            }
            //TODO: copy caught exceptions to main thread
            catch(AudioDataInvalidException& e) {
                std::stringstream str;
                str << "Decoder encountered error in data; ";
                if(auto* file = boost::get_error_info<ErrorTag::FilePath>(e))
                    str << "file: " << *file << ";";
                ERROR_LOG(logject) << str.str();
                stop();
                playman.currentPlaylist()->next();
                n = 0;
                play();
            }
            catch(boost::exception& e) {
                ERROR_LOG(logject) << "Exception caught; Diagnostics following";
                ERROR_LOG(logject) << boost::diagnostic_information(e);
                stop();
                playman.currentPlaylist()->next();
                n = 0;
                play();
            }
            catch(std::exception& e) {
                ERROR_LOG(logject) << e.what();
                stop();
                playman.currentPlaylist()->next();
                n = 0;
                play();
            }
        }

        stop();
        TRACE_LOG(logject) << "Thread ending";
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

    static Player::impl* stateMachine_;
    static Melosic::Playlist::Manager* playman_;

    StateChanged& stateChanged;

protected:
    decltype(stateMachine_) const& stateMachine = stateMachine_;
    decltype(playman_) const& playman = playman_;
};
Player::impl* State::stateMachine_(nullptr);
Melosic::Playlist::Manager* State::playman_(nullptr);

struct Error;
struct Playing;

struct Stopped : State {
    explicit Stopped(StateChanged& sc) : State(sc) {
        stateChanged(Output::DeviceState::Stopped);
    }

    void play() override {
        assert(stateMachine);
        assert(playman);
        assert(playman->currentPlaylist());
        if(!*playman->currentPlaylist())
            return;

        try {
            stateMachine->changeDevice();
            if(!stateMachine->sinkWrapper_)
                BOOST_THROW_EXCEPTION(std::exception());
            stateMachine->sink().prepareSink(playman->currentPlaylist()->currentTrack()->getAudioSpecs());
            stateMachine->sink().play();
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
        if(stateMachine->state() != Output::DeviceState::Error)
            stateMachine->play();
    }
    void pause() override {
        sinkChange();
        if(stateMachine->state() != Output::DeviceState::Error) {
            stateMachine->play();
            stateMachine->pause();
        }
    }
    void stop() override {
        sinkChange();
        if(stateMachine->state() != Output::DeviceState::Error)
            assert(stateMachine->state() == Output::DeviceState::Stopped);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Error;
    }

    void sinkChange() override {
        try {
            stateMachine->changeDevice();
            if(stateMachine->sinkWrapper_) {
                auto ptr = stateMachine->changeState<Stopped>();
                assert(ptr == this);
                assert(stateMachine->state() == Output::DeviceState::Stopped);
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
        assert(stateMachine);
        stateMachine->sink().stop();
        auto ptr = stateMachine->changeState<Stopped>();
        assert(ptr == this);
    }
};

struct Tell : virtual State {
    explicit Tell(StateChanged& sc) : State(sc) {}
    virtual chrono::milliseconds tell() const override {
        assert(playman);
        assert(playman->currentPlaylist());
        auto playlist = playman->currentPlaylist();
        assert(playlist);
        if(playlist->currentTrack() == playlist->end())
            return 0ms;
        return playlist->currentTrack()->tell();
    }
};

struct Paused;

struct Playing : virtual State, Stop, Tell {
    explicit Playing(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->sinkWrapper_);
        stateChanged(Output::DeviceState::Playing);
    }

    void pause() override {
        assert(stateMachine);
        stateMachine->sink().pause();
        auto ptr = stateMachine->changeState<Paused>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
//        assert(stateMachine);
//        assert(stateMachine->sink().state()==Output::DeviceState::Playing);
        return Output::DeviceState::Playing;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->sink().stop();
        stateMachine->changeDevice();
        stateMachine->play();
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
    }
};

struct Paused : virtual State, Stop, Tell {
    explicit Paused(StateChanged& sc) : State(sc), Stop(sc), Tell(sc) {
        assert(stateMachine->sinkWrapper_);
        stateChanged(Output::DeviceState::Paused);
    }

    void play() override {
        assert(stateMachine);
        stateMachine->sink().play();
        auto ptr = stateMachine->changeState<Playing>();
        assert(ptr == this);
    }

    Output::DeviceState state() const override {
        return Output::DeviceState::Paused;
    }

    void sinkChange() override {
        auto time(tell());
        stateMachine->sink().stop();
        stateMachine->changeDevice();
        stateMachine->play();
        stateMachine->playman.currentPlaylist()->currentTrack()->seek(time);
        stateMachine->pause();
    }
};

Player::impl::impl(Melosic::Playlist::Manager& playman, Output::Manager& outman) :
    playman(playman),
    outman(outman),
    end(false),
    playerThread(&impl::start, this),
    logject(logging::keywords::channel = "StateMachine")
{
    State::stateMachine_ = this;
    State::playman_ = &playman;
    currentState_ = std::make_shared<Stopped>(stateChanged);

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

void Player::impl::play() {
    unique_lock l(mu);
    auto cs = currentState_;
    assert(cs);
    l.unlock();
    cs->play();
}

void Player::impl::pause() {
    unique_lock l(mu);
    auto cs = currentState_;
    assert(cs);
    l.unlock();
    cs->pause();
}

void Player::impl::stop() {
    unique_lock l(mu);
    auto cs = currentState_;
    assert(cs);
    l.unlock();
    cs->stop();
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

chrono::milliseconds Player::impl::tell() {
    unique_lock l(mu);
    auto cs = currentState_;
    assert(cs);
    l.unlock();
    return cs->tell();
}

Output::DeviceState Player::impl::state() {
    lock_guard l(mu);
    auto cs = currentState_;
    assert(cs);
    return cs->state();
}

Output::PlayerSink& Player::impl::sink() {
    return sinkWrapper_;
}

void Player::impl::trackChangeSlot(const Track& track) {
    auto cp = playman.currentPlaylist();
    assert(cp);
    try {
        if(*cp && sinkWrapper_) {
            assert(track == *cp->currentTrack());
            if(sinkWrapper_.currentSpecs() != track.getAudioSpecs()) {
                LOG(logject) << "Changing sink specs for new track";
                sinkWrapper_.stop();
                this_thread::sleep_for(100ms);
                sinkWrapper_.prepareSink(const_cast<Track&>(track).getAudioSpecs());
                this_thread::sleep_for(100ms);
                sinkWrapper_.play();
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
    lock_guard l(sinkWrapper_.mu);
    sinkWrapper_.device = std::move(outman.createPlayerSink());
}

void Player::impl::sinkChangeSlot() {
    TRACE_LOG(logject) << "sinkChangeSlot()";
    try {
        unique_lock l(mu);
        auto cs = currentState_;
        assert(cs);
        l.unlock();
        cs->sinkChange();
    }
    catch(...) {
        ERROR_LOG(logject) << boost::current_exception_diagnostic_information();

        stop();
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

