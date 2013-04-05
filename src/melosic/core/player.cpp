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

#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>
namespace io = boost::iostreams;

#include <opqit/opaque_iterator.hpp>

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
#include "player.hpp"

namespace Melosic {
namespace Core {

class Player::impl {
public:
    impl(Slots::Manager& slotman, Output::Manager& outman)
        : slotman(slotman),
          stateChanged(slotman.get<Signals::Player::StateChanged>()),
          notifyPlayPosition(slotman.get<Signals::Player::NotifyPlayPos>()),
          playlistChanged(slotman.get<Signals::Playlist::PlaylistChanged>()),
          end_(false),
          playerThread(&impl::start, this),
          logject(logging::keywords::channel = "Player")
    {
        slotman.get<Signals::Output::PlayerSinkChanged>().connect([this,&outman]() {
            try {
                chrono::milliseconds time(0);
                auto state_ = state();
                if(state_ != Output::DeviceState::Stopped && state_ != Output::DeviceState::Error) {
                    time = tell();
                    stop();
                }

                changeOutput(outman.getPlayerSink());

                switch(state_) {
                    case DeviceState::Playing:
                        play();
                        seek(time);
                        break;
                    case DeviceState::Paused:
                        play();
                        seek(time);
                        pause();
                        break;
                    case DeviceState::Error:
                    case DeviceState::Stopped:
                    case DeviceState::Ready:
                    default:
                        break;
                }
            }
            catch(std::exception& e) {
                LOG(logject) << "Exception caught: " << e.what();

                stop();
            }
        });
    }

    ~impl() {
        if(!end()) {
            lock_guard l(m);
            end_ = true;
        }
        if(playerThread.joinable()) {
            TRACE_LOG(logject) << "Closing thread";
            playerThread.join();
        }
    }

    void play() {
        TRACE_LOG(logject) << "Play...";
        if(device && playlist) {
            if(device->state() == DeviceState::Stopped || device->state() == DeviceState::Error) {
                if(*playlist) {
                    this->device->prepareSink(playlist->currentTrack()->getAudioSpecs());
                }
            }
            device->play();
            stateChanged(DeviceState::Playing);
        }
        else {
            std::stringstream str;
            if(!device)
                str << ": device not initialised";
            if(!playlist)
                str << ": playlist not initialised";
            WARN_LOG(logject) << "Playback not possible" << str.str();
        }
    }

    void pause() {
        TRACE_LOG(logject) << "Pause...";
        if(device) {
            device->pause();
            stateChanged(DeviceState::Paused);
        }
    }

    void stop() {
        TRACE_LOG(logject) << "Stop...";
        if(device) {
            device->stop();
            stateChanged(DeviceState::Stopped);
        }
        if(playlist && *playlist) {
            playlist->currentTrack()->reset();
            playlist->currentTrack()->close();
        }
    }

    DeviceState state() {
        if(device) {
            return device->state();
        }
        return DeviceState::Error;
    }

    void seek(chrono::milliseconds dur) {
        TRACE_LOG(logject) << "Seek...";
        if(playlist) {
            playlist->seek(dur);
        }
    }

    chrono::milliseconds tell() {
        TRACE_LOG(logject) << "Tell...";
        if(playlist && *playlist) {
            return playlist->currentTrack()->tell();
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
        auto tmp = this->device.release();
        this->device = std::move(device);

        l.unlock();
        if(playlist && *playlist) {
            TRACE_LOG(logject) << "Initiallising new output device";
            this->device->prepareSink(playlist->currentTrack()->getAudioSpecs());
        }

        if(tmp) {
            TRACE_LOG(logject) << "Removing old output device";
            if(tmp->state() == DeviceState::Playing) {
                TRACE_LOG(logject) << "Resuming playback on new output device";
                this->device->play();
                stateChanged(DeviceState::Playing);
            }
            delete tmp;
        }
    }

    explicit operator bool() {
        return bool(device);
    }

    void openPlaylist(std::shared_ptr<Playlist> /*playlist*/) {
        lock_guard l(m);
//        if(this->playlist != playlist) {
//            if(this->playlist && *(this->playlist)) {
//                this->playlist->currentTrack()->close();
//            }
//            this->playlist = playlist;
//            slotman.get<Signals::Playlist::TrackChanged>()
//                    .emplace_connect(&impl::onTrackChangeSlot, this, ph::_1);
//            playlistChanged(playlist);
//        }
    }

    std::shared_ptr<Playlist> currentPlaylist() {
        lock_guard l(m);
        return playlist;
    }

private:
    void onTrackChangeSlot(const Track&) {
        if(playlist && *playlist && device) {
            if(device->currentSpecs() != currentPlaylist()->currentTrack()->getAudioSpecs()) {
                device->stop();
                this_thread::sleep_for(chrono::milliseconds(100));
                device->prepareSink(currentPlaylist()->currentTrack()->getAudioSpecs());
                this_thread::sleep_for(chrono::milliseconds(100));
                device->play();
            }
        }
    }

    bool end() {
        lock_guard l(m);
        return end_;
    }

    void start() {
        TRACE_LOG(logject) << "Thread starting";
        std::array<uint8_t,16384> s;
        std::streamsize n = 0;
        while(!end()) {
            try {
                if(device && playlist) {
                    switch(device->state()) {
                        case DeviceState::Playing: {
//                            std::clog << "play pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                            if(n < 1024) {
                                auto a = io::read(*playlist, (char*)s.data(), s.size());
                                if(bool(*playlist)) {
                                    notifyPlayPosition(playlist->currentTrack()->tell(),
                                                       playlist->currentTrack()->duration());
                                }
                                if(a == -1) {
                                    if(n == 0) {
                                        stop();
                                        playlist->currentTrack() = playlist->begin();
                                    }
                                    break;
                                }
                                else {
                                    n += a;
                                }
                            }
                            n -= io::write(*device, (char*)s.data()+s.size()-n, n);
                            break;
                        }
                        case DeviceState::Error:
                            ERROR_LOG(logject) << "Irrecoverable error occured";
                            device.reset();
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
                if(auto* file = boost::get_error_info<ErrorTag::FilePath>(e)) {
                    str << "file: " << *file << ";";
                }
                ERROR_LOG(logject) << str.str();
                stop();
                playlist->next();
                n = 0;
                play();
            }
            catch(boost::exception& e) {
                ERROR_LOG(logject) << "Exception caught; Diagnostics following";
                ERROR_LOG(logject) << boost::diagnostic_information(e);
                stop();
                playlist->next();
                n = 0;
                play();
            }
            catch(std::exception& e) {
                ERROR_LOG(logject) << e.what();
                stop();
                playlist->next();
                n = 0;
                play();
            }

            this_thread::sleep_for(chrono::milliseconds(10));
        }

        stop();
        TRACE_LOG(logject) << "Thread ending";
    }

    Slots::Manager& slotman;
    Signals::Player::StateChanged& stateChanged;
    Signals::Player::NotifyPlayPos& notifyPlayPosition;
    Signals::Playlist::PlaylistChanged& playlistChanged;
    std::shared_ptr<Playlist> playlist;
    std::unique_ptr<Output::PlayerSink> device;
    bool end_;
    thread playerThread;
    Logger::Logger logject;
    typedef mutex Mutex;
    Mutex m;
    friend class Player;
};

Player::Player(Slots::Manager& slotman, Output::Manager& outman) : pimpl(new impl(slotman, outman)) {}

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

DeviceState Player::state() {
    return pimpl->state();
}

void Player::seek(chrono::milliseconds dur) {
    pimpl->seek(dur);
}

chrono::milliseconds Player::tell() {
    return pimpl->tell();
}

void Player::finish() {
    pimpl->finish();
}

void Player::changeOutput(std::unique_ptr<Output::PlayerSink> device) {
    pimpl->changeOutput(std::move(device));
}

Player::operator bool() {
    return bool(*pimpl);
}

void Player::openPlaylist(std::shared_ptr<Playlist> playlist) {
    pimpl->openPlaylist(playlist);
}

std::shared_ptr<Playlist> Player::currentPlaylist() {
    return pimpl->currentPlaylist();
}

} // namespace Core
} // namespace Melosic

