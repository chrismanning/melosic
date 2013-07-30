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
#include <melosic/core/statemachine.hpp>

#include "player.hpp"

namespace Melosic {
namespace Core {

struct Stopped;

struct NotifyPlayPos : Signals::Signal<Signals::Player::NotifyPlayPos> {
    using Super::Signal;
};

class Player::impl {
public:
    impl(Melosic::Playlist::Manager& playlistman, Output::Manager& outman) :
        playlistman(playlistman),
        stateMachine(playlistman, outman),
        end(false),
        playerThread(&impl::start, this)
    {
        playlistman.getTrackChangedSignal().connect(&StateMachine::trackChangeSlot, &stateMachine);

        outman.getPlayerSinkChangedSignal().connect(&StateMachine::sinkChangeSlot, &stateMachine);
    }

    ~impl() {
        end = true;
        if(playerThread.joinable()) {
            TRACE_LOG(logject) << "Closing thread";
            playerThread.join();
        }
    }

    void play() {
        TRACE_LOG(logject) << "Play...";
        stateMachine.play();
    }

    void pause() {
        TRACE_LOG(logject) << "Pause...";
        stateMachine.pause();
    }

    void stop() {
        TRACE_LOG(logject) << "Stop...";
        stateMachine.stop();
        if(playlistman.currentPlaylist() && *playlistman.currentPlaylist()) {
            playlistman.currentPlaylist()->currentTrack()->reset();
            playlistman.currentPlaylist()->currentTrack()->close();
        }
    }

    DeviceState state() {
        return stateMachine.state();
    }

    void seek(chrono::milliseconds dur) {
        TRACE_LOG(logject) << "Seek...";
        if(playlistman.currentPlaylist()) {
            playlistman.currentPlaylist()->seek(dur);
        }
    }

    chrono::milliseconds tell() {
        TRACE_LOG(logject) << "Tell...";
        return stateMachine.tell();
    }

    std::shared_ptr<Playlist> currentPlaylist() {
        lock_guard l(m);
        return playlistman.currentPlaylist();
    }

    Signals::Player::StateChanged& stateChangedSignal() {
        return stateMachine.stateChangedSignal();
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
                playlist = playlistman.currentPlaylist();
                if(!playlist) {
                    ERROR_LOG(logject) << "No playlist";
                    continue;
                }
                switch(stateMachine.state()) {
                    case DeviceState::Playing: {
//                            std::clog << "play pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                        if(n < 1024) {
                            auto a = io::read(*playlist, (char*)s.data(), s.size());
                            if(static_cast<bool>(*playlist))
                                notifyPlayPosition(playlist->currentTrack()->tell(),
                                                   playlist->currentTrack()->duration());

                            if(a == -1) {
                                if(n == 0) {
                                    stateMachine.stop();
                                    playlist->currentTrack() = playlist->begin();
                                }
                                break;
                            }
                            else
                                n += a;
                        }
                        TRACE_LOG(logject) << "playing... " << stateMachine.tell().count();
                        n -= io::write(stateMachine.sink(), (char*)s.data()+s.size()-n, n);
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
                stateMachine.stop();
                playlistman.currentPlaylist()->next();
                n = 0;
                stateMachine.play();
            }
            catch(boost::exception& e) {
                ERROR_LOG(logject) << "Exception caught; Diagnostics following";
                ERROR_LOG(logject) << boost::diagnostic_information(e);
                stateMachine.stop();
                playlistman.currentPlaylist()->next();
                n = 0;
                stateMachine.play();
            }
            catch(std::exception& e) {
                ERROR_LOG(logject) << e.what();
                stateMachine.stop();
                playlistman.currentPlaylist()->next();
                n = 0;
                stateMachine.play();
            }
        }

        stateMachine.stop();
        TRACE_LOG(logject) << "Thread ending";
    }

    Melosic::Playlist::Manager& playlistman;
    StateMachine stateMachine;
    NotifyPlayPos notifyPlayPosition;
    std::atomic_bool end;
    thread playerThread;
    Logger::Logger logject{logging::keywords::channel = "Player"};
    typedef mutex Mutex;
    Mutex m;
    friend class Player;
};

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
    return pimpl->stateChangedSignal();
}

} // namespace Core
} // namespace Melosic

