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
#include <boost/thread.hpp>

#include "track.hpp"
#include <melosic/common/stream.hpp>
#include <melosic/managers/input/pluginterface.hpp>
#include <melosic/common/common.hpp>

namespace Melosic {

class Track::impl {
public:
    impl(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory) :
        impl(std::move(input), factory, std::chrono::milliseconds(0)) {}
    impl(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory, std::chrono::milliseconds offset) :
        input(std::move(input)),
        factory(factory),
        offset(offset)
    {
        this->input->seek(0, std::ios_base::beg, std::ios_base::in);
        decoder = factory(*this->input);
        this->input->close();
    }

    ~impl() {}

    bool isOpen() {
        return input->isOpen();
    }

    void reOpen() {
        input->reOpen();
        reset();
        std::lock_guard<Mutex> l(mu);
        decoder->seek(offset);
    }

    void close() {
        input->close();
    }

    std::streamsize read(char * s, std::streamsize n) {
        if(!isOpen()) {
            reOpen();
        }
        std::lock_guard<Mutex> l(mu);
        return decoder->read(s, n);
    }

    Track::TagsType& getTags() {
        return tags;
    }

    void seek(std::chrono::milliseconds dur) {
        if(!isOpen()) {
            reOpen();
        }
        std::lock_guard<Mutex> l(mu);
        decoder->seek(dur + offset);
    }

    std::chrono::milliseconds tell() {
        if(!isOpen()) {
            reOpen();
        }
        boost::shared_lock<Mutex> l(mu);
        return decoder->tell();
    }

    void reset() {
        std::lock_guard<Mutex> l(mu);
        decoder->reset();
    }

    std::chrono::milliseconds duration() {
        auto& as = getAudioSpecs();
        return std::chrono::milliseconds(uint64_t(as.total_samples / (as.sample_rate/1000.0)));
    }

    AudioSpecs& getAudioSpecs() {
        boost::shared_lock<Mutex> l(mu);
        return decoder->getAudioSpecs();
    }

    explicit operator bool() {
        boost::shared_lock<Mutex> l(mu);
        if(decoder) {
            return bool(*decoder);
        }
        else
            return false;
    }

private:
    std::unique_ptr<IO::BiDirectionalClosableSeekable> input;
    Input::Factory factory;
    std::chrono::milliseconds offset;
    std::shared_ptr<Input::ISource> decoder;
    std::once_flag decoderInitFlag;
    Track::TagsType tags;
    typedef boost::shared_mutex Mutex;
    Mutex mu;
};

Track::Track(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory) :
    Track(std::move(input), factory, std::chrono::milliseconds(0)) {}
Track::Track(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory, std::chrono::milliseconds offset) :
    pimpl(new impl(std::move(input), factory, offset)) {}

Track::~Track() {}

std::streamsize Track::do_read(char * s, std::streamsize n) {
    return pimpl->read(s, n);
}

void Track::do_close() {
    pimpl->close();
}

bool Track::do_isOpen() {
    return pimpl->isOpen();
}

void Track::do_reOpen() {
    pimpl->reOpen();
}

Track::TagsType& Track::getTags() {
    return pimpl->getTags();
}

void Track::seek(std::chrono::milliseconds dur) {
    pimpl->seek(dur);
}

std::chrono::milliseconds Track::tell() {
    return pimpl->tell();
}

void Track::reset() {
    pimpl->reset();
}

std::chrono::milliseconds Track::duration() {
    return pimpl->duration();
}

Melosic::AudioSpecs& Track::getAudioSpecs() {
    return pimpl->getAudioSpecs();
}

Track::operator bool() {
    return bool(*pimpl);
}

} //end namespace Melosic
