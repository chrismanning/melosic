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
namespace this_thread = std::this_thread;
using std::thread; using std::mutex;
using unique_lock = std::unique_lock<mutex>;
using lock_guard = std::lock_guard<mutex>;
#include <atomic>

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
#include <melosic/melin/sigslots/signals_fwd.hpp>
#include <melosic/melin/sigslots/signals.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include <melosic/melin/playlist.hpp>
#include <melosic/core/kernel.hpp>

#include "player.hpp"

template <typename T, typename T2>
bool operator==(std::unique_ptr<T>& ptr, T2* raw) {
    static_assert(std::is_base_of<T,T2>::value || std::is_base_of<T2,T>::value, "pointer types must be comparable");
    return ptr.get() == raw;
}
template <typename T, typename T2>
bool operator==(T2* raw, std::unique_ptr<T>& ptr) {
    return ptr == raw;
}

namespace Melosic {
namespace Core {

struct StateChanged : Signals::Signal<void(Output::DeviceState)> {
    using super::Signal;
};

struct Stopped;

struct State {
    State(Player::impl& player, Melosic::Playlist::Manager& playman) :
        player(player),
        playman(playman)
    {}

    virtual ~State() {}

    //default actions (do nothing)
    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}

    virtual chrono::milliseconds tell() const {
        return chrono::milliseconds::zero();
    }

    virtual void sinkChanged(Output::Manager& outman);

    std::unique_ptr<Output::PlayerSink>& device() {
        return device_;
    }

protected:
    Player::impl& player;
    Melosic::Playlist::Manager& playman;
    std::unique_ptr<Output::PlayerSink> device_;
    static StateChanged stateChanged;
};
StateChanged State::stateChanged;

class Player::impl {
public:
    impl(Melosic::Playlist::Manager& playlistman, Output::Manager& outman, Slots::Manager& slotman) :
        currentState(new State(*this, playlistman)),
        playlistman(playlistman),
        slotman(slotman),
        stateChanged(slotman.get<Signals::Player::StateChanged>()),
        notifyPlayPosition(slotman.get<Signals::Player::NotifyPlayPos>()),
        end_(false),
        playerThread(&impl::start, this),
        logject(logging::keywords::channel = "Player")
    {
        this->slotman.get<Signals::Output::PlayerSinkChanged>().connect([this,&outman]() {
            try {
                currentState->sinkChanged(outman);
            }
            catch(std::exception& e) {
                LOG(logject) << "Exception caught: " << e.what();

                stop();
            }
        });
    }

    ~impl() {
        end_ = true;
        if(playerThread.joinable()) {
            TRACE_LOG(logject) << "Closing thread";
            playerThread.join();
        }
    }

    void play() {
        TRACE_LOG(logject) << "Play...";
        if(currentState->device() && playlistman.currentPlaylist()) {
            if((currentState->device()->state() == DeviceState::Stopped || currentState->device()->state() == DeviceState::Error) && *playlistman.currentPlaylist())
                currentState->device()->prepareSink(playlistman.currentPlaylist()->currentTrack()->getAudioSpecs());

            currentState->device()->play();
            stateChanged(DeviceState::Playing);
        }
        else {
            std::stringstream str;
            if(!currentState->device())
                str << ": device not initialised";
            if(!playlistman.currentPlaylist())
                str << ": playlist not initialised";
            WARN_LOG(logject) << "Playback not possible" << str.str();
        }
    }

    void pause() {
        TRACE_LOG(logject) << "Pause...";
        if(currentState->device()) {
            currentState->device()->pause();
            stateChanged(DeviceState::Paused);
        }
    }

    void stop() {
        TRACE_LOG(logject) << "Stop...";
        if(currentState->device()) {
            currentState->device()->stop();
            stateChanged(DeviceState::Stopped);
        }
        if(playlistman.currentPlaylist() && *playlistman.currentPlaylist()) {
            playlistman.currentPlaylist()->currentTrack()->reset();
            playlistman.currentPlaylist()->currentTrack()->close();
        }
    }

    DeviceState state() {
        if(currentState->device()) {
            return currentState->device()->state();
        }
        return DeviceState::Error;
    }

    void seek(chrono::milliseconds dur) {
        TRACE_LOG(logject) << "Seek...";
        if(playlistman.currentPlaylist()) {
            playlistman.currentPlaylist()->seek(dur);
        }
    }

    chrono::milliseconds tell() {
        TRACE_LOG(logject) << "Tell...";
        if(playlistman.currentPlaylist() && *playlistman.currentPlaylist()) {
            return playlistman.currentPlaylist()->currentTrack()->tell();
        }
        return chrono::seconds(0);
    }

    void finish() {
        if(playerThread.joinable()) {
            playerThread.join();
        }
    }

    void changeOutput(std::unique_ptr<Output::PlayerSink> device) {
        TRACE_LOG(logject) << "Attempting to change output device";
        unique_lock l(m);
        auto tmp = this->currentState->device().release();
        this->currentState->device() = std::move(device);

        l.unlock();
        if(playlistman.currentPlaylist() && *playlistman.currentPlaylist()) {
            TRACE_LOG(logject) << "Initiallising new output device";
            this->currentState->device()->prepareSink(playlistman.currentPlaylist()->currentTrack()->getAudioSpecs());
        }

        if(tmp) {
            TRACE_LOG(logject) << "Removing old output device";
            if(tmp->state() == DeviceState::Playing) {
                TRACE_LOG(logject) << "Resuming playback on new output device";
                this->currentState->device()->play();
                stateChanged(DeviceState::Playing);
            }
            delete tmp;
        }
    }

    explicit operator bool() {
        return static_cast<bool>(currentState->device());
    }

    std::shared_ptr<Playlist> currentPlaylist() {
        lock_guard l(m);
        return playlistman.currentPlaylist();
    }

    std::unique_ptr<State> currentState;
    template <typename S>
    auto changeState() -> decltype(currentState) {
        static_assert(std::is_base_of<State, S>::value, "Template parameter must be State derived");

        auto r = std::move(currentState);
        currentState.reset(new S(*this, playlistman));
        std::swap(r->device(), currentState->device());
        return std::move(r);
    }

private:
    void onTrackChangeSlot(const Track&) {
        if(playlistman.currentPlaylist() && *playlistman.currentPlaylist() && currentState->device()) {
            if(currentState->device()->currentSpecs() != currentPlaylist()->currentTrack()->getAudioSpecs()) {
                currentState->device()->stop();
                this_thread::sleep_for(chrono::milliseconds(100));
                currentState->device()->prepareSink(currentPlaylist()->currentTrack()->getAudioSpecs());
                this_thread::sleep_for(chrono::milliseconds(100));
                currentState->device()->play();
            }
        }
    }

    bool end() {
        return end_;
    }

    void start() {
        TRACE_LOG(logject) << "Thread starting";
        std::array<uint8_t,16384> s;
        std::streamsize n = 0;
        while(!end()) {
            try {
                if(currentState->device() && playlistman.currentPlaylist()) {
                    switch(currentState->device()->state()) {
                        case DeviceState::Playing: {
//                            std::clog << "play pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                            if(n < 1024) {
                                auto a = io::read(*playlistman.currentPlaylist(), (char*)s.data(), s.size());
                                if(static_cast<bool>(*playlistman.currentPlaylist()))
                                    notifyPlayPosition(playlistman.currentPlaylist()->currentTrack()->tell(),
                                                       playlistman.currentPlaylist()->currentTrack()->duration());

                                if(a == -1) {
                                    if(n == 0) {
                                        stop();
                                        playlistman.currentPlaylist()->currentTrack() = playlistman.currentPlaylist()->begin();
                                    }
                                    break;
                                }
                                else
                                    n += a;
                            }
                            n -= io::write(*currentState->device(), (char*)s.data()+s.size()-n, n);
                            break;
                        }
                        case DeviceState::Error:
                            ERROR_LOG(logject) << "Irrecoverable error occured";
                            currentState->device().reset();
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
            }
            //TODO: copy caught exceptions to main thread
            catch(AudioDataInvalidException& e) {
                std::stringstream str;
                str << "Decoder encountered error in data; ";
                if(auto* file = boost::get_error_info<ErrorTag::FilePath>(e))
                    str << "file: " << *file << ";";
                ERROR_LOG(logject) << str.str();
                stop();
                playlistman.currentPlaylist()->next();
                n = 0;
                play();
            }
            catch(boost::exception& e) {
                ERROR_LOG(logject) << "Exception caught; Diagnostics following";
                ERROR_LOG(logject) << boost::diagnostic_information(e);
                stop();
                playlistman.currentPlaylist()->next();
                n = 0;
                play();
            }
            catch(std::exception& e) {
                ERROR_LOG(logject) << e.what();
                stop();
                playlistman.currentPlaylist()->next();
                n = 0;
                play();
            }

            this_thread::sleep_for(chrono::milliseconds(10));
        }

        stop();
        TRACE_LOG(logject) << "Thread ending";
    }

    Melosic::Playlist::Manager& playlistman;
    Slots::Manager& slotman;
    Signals::Player::StateChanged& stateChanged;
    Signals::Player::NotifyPlayPos& notifyPlayPosition;
//    std::shared_ptr<Playlist> playlist;
//    std::unique_ptr<Output::PlayerSink> device;
    std::atomic<bool> end_;
    thread playerThread;
    Logger::Logger logject;
    typedef mutex Mutex;
    Mutex m;
    friend class Player;
};

void State::sinkChanged(Output::Manager& outman) {
    device_ = std::move(outman.getPlayerSink());
    if(device_) {
        auto ptr = player.changeState<Stopped>();
        assert(ptr == this);
    }
}

Player::Player(Melosic::Playlist::Manager& playlistman, Output::Manager& outman, Slots::Manager& slotman) :
    pimpl(new impl(playlistman, outman, slotman)) {}

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

void Player::finish() {
    pimpl->finish();
}

void Player::changeOutput(std::unique_ptr<Output::PlayerSink> device) {
    pimpl->changeOutput(std::move(device));
}

Player::operator bool() const {
    return static_cast<bool>(*pimpl);
}

struct Stopped;

struct Stop : virtual State {
    using State::State;

    virtual void stop() override {
        assert(device_);
        device_->stop();
        auto ptr = player.changeState<Stopped>();
        assert(ptr == this);
    }
};

struct Tell : virtual State {
    using State::State;

    virtual chrono::milliseconds tell() const override {
        return playman.currentPlaylist()->currentTrack()->tell();
    }
};

struct Paused;

struct Playing : virtual State, Stop, Tell {
    Playing(Player::impl& player, Melosic::Playlist::Manager& playman) :
        State(player, playman),
        Stop(player, playman),
        Tell(player, playman)
    {
        stateChanged(Output::DeviceState::Playing);
    }

    virtual void pause() override {
        assert(device_);
        device_->pause();
        auto ptr = player.changeState<Paused>();
        assert(ptr == this);
    }

    virtual void sinkChanged(Output::Manager& outman) override {
        chrono::milliseconds time(tell());
        auto ptr = player.changeState<Stopped>();
        assert(ptr == this);
        player.currentState->sinkChanged(outman);
        player.currentState->play();
        player.seek(time);
    }
};

struct Paused : virtual State, Stop, Tell {
    Paused(Player::impl& player, Melosic::Playlist::Manager& playman) :
        State(player, playman),
        Stop(player, playman),
        Tell(player, playman)
    {
        stateChanged(Output::DeviceState::Paused);
    }

    virtual void play() override {
        assert(device_);
        device_->play();
        auto ptr = player.changeState<Playing>();
        assert(ptr == this);
    }

    virtual void sinkChanged(Output::Manager& outman) override {
        chrono::milliseconds time(tell());
        auto ptr = player.changeState<Stopped>();
        assert(ptr == this);
        player.currentState->sinkChanged(outman);
        player.currentState->play();
        player.seek(time);
        player.currentState->pause();
    }
};

struct Stopped : State {
    Stopped(Player::impl& player, Melosic::Playlist::Manager& playman) :
        State(player, playman)
    {
        stateChanged(Output::DeviceState::Stopped);
    }

    virtual void play() override {
        assert(device_);
        device_->prepareSink(playman.currentPlaylist()->currentTrack()->getAudioSpecs());
        device_->play();
        auto ptr = player.changeState<Playing>();
        assert(ptr == this);
    }

    virtual void sinkChanged(Output::Manager& outman) override {
        device_ = std::move(outman.getPlayerSink());
        if(!device_) {
            auto ptr = player.changeState<State>();
            assert(ptr == this);
        }
    }
};

} // namespace Core
} // namespace Melosic

