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
#include <chrono>
#include <iostream>
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>
namespace io = boost::iostreams;

#include <melosic/core/player.hpp>
#include <melosic/core/playlist.hpp>
#include <melosic/core/track.hpp>
#include <melosic/managers/input/pluginterface.hpp>
#include <melosic/managers/output/pluginterface.hpp>
#include <melosic/common/common.hpp>

namespace Melosic {

class Player::impl {
public:
    impl() : impl(nullptr) {}

    impl(std::unique_ptr<Output::DeviceSink> device) :
        device(std::move(device)),
        end_(false),
        playerThread(&impl::start, this),
        logject(boost::log::keywords::channel = "Player")
    {}

    ~impl() {
        if(!end()) {
            std::lock_guard<std::mutex> l(m);
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
                    this->device->prepareSink(playlist->current()->getAudioSpecs());
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
            playlist->current()->reset();
            playlist->current()->close();
        }
    }

    DeviceState state() {
        if(device) {
            return device->state();
        }
        return DeviceState::Error;
    }

    void seek(std::chrono::milliseconds dur) {
        TRACE_LOG(logject) << "Seek...";
        if(playlist) {
            playlist->seek(dur);
        }
    }

    std::chrono::milliseconds tell() {
        TRACE_LOG(logject) << "Tell...";
        if(playlist && *playlist) {
            return playlist->current()->tell();
        }
        return std::chrono::milliseconds(0);
    }

    void finish() {
        if(playerThread.joinable()) {
            playerThread.join();
        }
    }

    void changeOutput(std::unique_ptr<Output::DeviceSink> device) {
        std::unique_lock<std::mutex> l(m);
        auto tmp = this->device.release();
        this->device = std::move(device);

        l.unlock();
        if(playlist && *playlist) {
            this->device->prepareSink(playlist->current()->getAudioSpecs());
        }

        if(tmp) {
            if(tmp->state() == DeviceState::Playing) {
                this->device->play();
                stateChangedSig(DeviceState::Playing);
            }
            delete tmp;
        }
    }

    operator bool() {
        return bool(device);
    }

    void openPlaylist(boost::shared_ptr<Playlist> playlist) {
        std::lock_guard<std::mutex> l(m);
        if(this->playlist != playlist) {
            if(this->playlist && *(this->playlist)) {
                this->playlist->current()->close();
            }
            this->playlist = playlist;
            playlist->connectTrackChanged(boost::bind(&impl::onTrackChangeSlot, this));
        }
    }

    boost::shared_ptr<Playlist> currentPlaylist() {
        std::lock_guard<std::mutex> l(m);
        return playlist;
    }

    boost::signals2::connection connectState(const StateSignal::slot_type& slot) {
        return stateChangedSig.connect(slot);
    }

    boost::signals2::connection connectNotifySlot(const NotifySignal::slot_type& slot) {
        return notifySig.connect(slot);
    }

private:
    void onTrackChangeSlot() {
        if(playlist && *playlist) {
            if(device->currentSpecs() != currentPlaylist()->current()->getAudioSpecs()) {
                stop();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                device->prepareSink(currentPlaylist()->current()->getAudioSpecs());
                play();
            }
        }
    }

    bool end() {
        std::lock_guard<std::mutex> l(m);
        return end_;
    }

    void start() {
        TRACE_LOG(logject) << "Thread starting";
        std::vector<unsigned char> s(16384);
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
                                    notifySig(playlist->current()->tell(), playlist->current()->duration());
                                }
                                if(a == -1) {
                                    if(n == 0) {
                                        stop();
                                        playlist->current() = playlist->begin();
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
            catch(Exception& e) {
                ERROR_LOG(logject) << e.what();
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
            std::chrono::milliseconds dur(10);
            std::this_thread::sleep_for(dur);
        }

        stop();
        TRACE_LOG(logject) << "Thread ending";
    }

    boost::shared_ptr<Playlist> playlist;
    std::unique_ptr<Output::DeviceSink> device;
    bool end_;
    std::thread playerThread;
    Logger::Logger logject;
    std::mutex m;
    StateSignal stateChangedSig;
    NotifySignal notifySig;
};

Player::Player() : pimpl(new impl) {}

Player::Player(std::unique_ptr<Output::DeviceSink> device)
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

void Player::seek(std::chrono::milliseconds dur) {
    pimpl->seek(dur);
}

std::chrono::milliseconds Player::tell() {
    return pimpl->tell();
}

void Player::finish() {
    pimpl->finish();
}

void Player::changeOutput(std::unique_ptr<Output::DeviceSink> device) {
    pimpl->changeOutput(std::move(device));
}

Player::operator bool() {
    return bool(*pimpl);
}

void Player::openPlaylist(boost::shared_ptr<Playlist> playlist) {
    pimpl->openPlaylist(playlist);
}

boost::shared_ptr<Playlist> Player::currentPlaylist() {
    return pimpl->currentPlaylist();
}

boost::signals2::connection Player::connectState(const StateSignal::slot_type& slot) {
    return pimpl->connectState(slot);
}

boost::signals2::connection Player::connectNotifySlot(const NotifySignal::slot_type& slot) {
    return pimpl->connectNotifySlot(slot);
}

}//end namespace Melosic

