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
#include <boost/iostreams/copy.hpp>
namespace io = boost::iostreams;

#include <melosic/core/player.hpp>
#include <melosic/managers/input/pluginterface.hpp>
#include <melosic/managers/output/pluginterface.hpp>
#include <melosic/common/common.hpp>

namespace Melosic {

using Output::DeviceState;

class Player::impl {
public:
    impl(Input::ISource& stream, std::unique_ptr<Output::IDeviceSink> device) :
        stream(stream),
        device(std::move(device)),
        end_(false),
        playerThread(&impl::start, this)
    {}

    ~impl() {
        if(!end()) {
            std::lock_guard<std::mutex> l(m);
            end_ = true;
        }
        if(playerThread.joinable()) {
            std::cerr << "Closing thread\n";
            playerThread.join();
        }
    }

    void play() {
        std::cerr << "Play...\n";
        if(device->state() == DeviceState::Stopped || device->state() == DeviceState::Error)
            this->device->prepareDevice(stream.getAudioSpecs());
        device->play();
    }

    void pause() {
        std::cerr << "Pause...\n";
        device->pause();
    }

    void stop() {
        std::cerr << "Stop...\n";
        device->stop();
    }

    void seek(std::chrono::milliseconds dur) {
        std::cerr << "Seek...\n";
        stream.seek(dur);
    }

    std::chrono::milliseconds tell() {
        std::cerr << "Tell...\n";
        return stream.tell();
    }

    void finish() {
        if(playerThread.joinable()) {
            playerThread.join();
        }
    }

    void changeOutput(std::unique_ptr<Output::IDeviceSink> device) {
        std::lock_guard<std::mutex> l(m);
        auto tmp = this->device.release();
        this->device = std::move(device);
        this->device->prepareDevice(stream.getAudioSpecs());

        if(tmp->state() == DeviceState::Playing) {
            this->device->play();
        }
        delete tmp;
    }

private:
    bool end() {
        std::lock_guard<std::mutex> l(m);
        return end_;
    }

    void start() {
        std::cerr << "Thread starting\n";
            std::vector<unsigned char> s(16384);
            std::streamsize n = 0;
            while(!end()) {
                try {
                    switch(device->state()) {
                        case DeviceState::Playing: {
//                        std::clog << "play pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                            if(n == 0) {
                                n = io::read(stream, (char*)&s[0], s.size());
                                if(n == -1) {
                                    stop();
                                    break;
                                }
                            }
                            n -= io::write(*device, (char*)&s[s.size()-n], n);
                            break;
                        }
                        case DeviceState::Error:
                            stop();
                            break;
                        case DeviceState::Stopped:
                            n = 0;
                            stream.seek(std::chrono::seconds(0));
                        case DeviceState::Paused:
//                        std::clog << "paused pos: " << stream.seek(0, std::ios_base::cur) << std::endl;
                        case DeviceState::Ready:
                        default:
                            break;
                    }
                }
                catch(std::exception& e) {
                    std::clog << e.what() << std::endl;
                }
                std::chrono::milliseconds dur(10);
                std::this_thread::sleep_for(dur);
            }

        stop();
        std::cerr << "Thread ending\n";
    }

    Input::ISource& stream;
    std::unique_ptr<Output::IDeviceSink> device;
    bool end_;
    std::thread playerThread;
    std::mutex m;
};

Player::Player(Input::ISource& stream, std::unique_ptr<Output::IDeviceSink> device)
    : pimpl(new impl(stream, std::move(device))) {}

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

void Player::seek(std::chrono::milliseconds dur) {
    pimpl->seek(dur);
}

std::chrono::milliseconds Player::tell() {
    return pimpl->tell();
}

void Player::finish() {
    pimpl->finish();
}

void Player::changeOutput(std::unique_ptr<Output::IDeviceSink> device) {
    pimpl->changeOutput(std::move(device));
}

}//end namespace Melosic
