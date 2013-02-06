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
using std::thread; using std::mutex; using std::unique_lock; using std::lock_guard;
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>
namespace io = boost::iostreams;

#include <opqit/opaque_iterator.hpp>

#include <melosic/common/error.hpp>
#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/audiospecs.hpp>

namespace Melosic {

class Player::impl {
public:
    impl() : impl(nullptr) {}

    impl(std::unique_ptr<Output::PlayerSink> device) :
        device(std::move(device)),
        end_(false),
        playerThread(&impl::start, this),
        logject(boost::log::keywords::channel = "Player")
    {}

    ~impl() {
        if(!end()) {
            lock_guard<Mutex> l(m);
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
            stateChangedSig(DeviceState::Playing);
        }
    }

    void pause() {
        TRACE_LOG(logject) << "Pause...";
        if(device) {
            device->pause();
            stateChangedSig(DeviceState::Paused);
        }
    }

    void stop() {
        TRACE_LOG(logject) << "Stop...";
        if(device) {
            device->stop();
            stateChangedSig(DeviceState::Stopped);
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
        return chrono::milliseconds(0);
    }

    void finish() {
        if(playerThread.joinable()) {
            playerThread.join();
        }
    }

    void changeOutput(std::unique_ptr<Output::PlayerSink> device) {
        unique_lock<Mutex> l(m);
        auto tmp = this->device.release();
        this->device = std::move(device);

        l.unlock();
        if(playlist && *playlist) {
            this->device->prepareSink(playlist->currentTrack()->getAudioSpecs());
        }

        if(tmp) {
            if(tmp->state() == DeviceState::Playing) {
                this->device->play();
                stateChangedSig(DeviceState::Playing);
            }
            delete tmp;
        }
    }

    explicit operator bool() {
        return bool(device);
    }

    void openPlaylist(std::shared_ptr<Playlist> playlist) {
        lock_guard<Mutex> l(m);
        if(this->playlist != playlist) {
            if(this->playlist && *(this->playlist)) {
                this->playlist->currentTrack()->close();
            }
            this->playlist = playlist;
            playlist->connectTrackChangedSlot(boost::bind(&impl::onTrackChangeSlot, this));
            playlistChangedSig(playlist);
        }
    }

    std::shared_ptr<Playlist> currentPlaylist() {
        lock_guard<Mutex> l(m);
        return playlist;
    }

    boost::signals2::connection connectStateSlot(const StateSignal::slot_type& slot) {
        return stateChangedSig.connect(slot);
    }

    boost::signals2::connection connectNotifySlot(const NotifySignal::slot_type& slot) {
        return notifySig.connect(slot);
    }

    boost::signals2::connection connectPlaylistChangeSlot(const PlaylistChangeSignal::slot_type& slot) {
        return playlistChangedSig.connect(slot);
    }

private:
    void onTrackChangeSlot() {
        if(playlist && *playlist) {
            if(device->currentSpecs() != currentPlaylist()->currentTrack()->getAudioSpecs()) {
                stop();
                this_thread::sleep_for(chrono::milliseconds(10));
                device->prepareSink(currentPlaylist()->currentTrack()->getAudioSpecs());
                play();
            }
        }
    }

    bool end() {
        lock_guard<Mutex> l(m);
        return end_;
    }

    void start() {
        TRACE_LOG(logject) << "Thread starting";
        std::array<unsigned char,16384> s;
        std::streamsize n = 0;
        while(!end()) {
            try {
                if(device && playlist) {
                    switch(device->state()) {
                        case DeviceState::Playing: {
//                            std::clog << "play pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                            if(n < 1024) {
                                auto a = io::read(*playlist, (char*)&s[0], s.size());
                                if(bool(*playlist)) {
                                    notifySig(playlist->currentTrack()->tell(), playlist->currentTrack()->duration());
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
                            n -= io::write(*device, (char*)&s[s.size()-n], n);
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

    std::shared_ptr<Playlist> playlist;
    std::unique_ptr<Output::PlayerSink> device;
    bool end_;
    thread playerThread;
    Logger::Logger logject;
    typedef mutex Mutex;
    Mutex m;
    StateSignal stateChangedSig;
    NotifySignal notifySig;
    PlaylistChangeSignal playlistChangedSig;
};

Player::Player() : pimpl(new impl) {}

Player::Player(std::unique_ptr<Output::PlayerSink> device)
    : pimpl(new impl(std::move(device))) {}

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

boost::signals2::connection Player::connectStateSlot(const StateSignal::slot_type& slot) {
    return pimpl->connectStateSlot(slot);
}

boost::signals2::connection Player::connectNotifySlot(const NotifySignal::slot_type& slot) {
    return pimpl->connectNotifySlot(slot);
}

boost::signals2::connection Player::connectPlaylistChangeSlot(const PlaylistChangeSignal::slot_type& slot) {
    return pimpl->connectPlaylistChangeSlot(slot);
}

}//end namespace Melosic

