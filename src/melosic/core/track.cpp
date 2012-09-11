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
        decoder->seek(offset);
    }

    void close() {
        input->close();
    }

    std::streamsize read(char * s, std::streamsize n) {
        if(!isOpen()) {
            reOpen();
        }
        return decoder->read(s, n);
    }

    std::streampos seek(std::streamoff off,
                        std::ios_base::seekdir way,
                        std::ios_base::openmode which)
    {
        if(!isOpen()) {
            reOpen();
        }
        return decoder->seekg(off, way);
    }

    Track::TagsType& getTags() {
        return tags;
    }

    void seek(std::chrono::milliseconds dur) {
        if(!isOpen()) {
            reOpen();
        }
        decoder->seek(dur + offset);
    }

    std::chrono::milliseconds tell() {
        if(!isOpen()) {
            reOpen();
        }
        auto pos = input->seek(0, std::ios_base::cur, std::ios_base::in);

        return std::chrono::milliseconds((long)(pos / getAudioSpecs().sample_rate * 1000 / getAudioSpecs().bps));
    }

    AudioSpecs& getAudioSpecs() {
        return decoder->getAudioSpecs();
    }

    explicit operator bool() {
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
};

Track::Track(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory) :
    Track(std::move(input), factory, std::chrono::milliseconds(0)) {}
Track::Track(std::unique_ptr<IO::BiDirectionalClosableSeekable> input, Input::Factory factory, std::chrono::milliseconds offset) :
    pimpl(new impl(std::move(input), factory, offset)) {}

Track::~Track() {}

std::streamsize Track::do_read(char * s, std::streamsize n) {
    return pimpl->read(s, n);
}

std::streampos Track::do_seekg(std::streamoff off, std::ios_base::seekdir way) {
    return pimpl->seek(off, way, std::ios_base::in);
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

Melosic::AudioSpecs& Track::getAudioSpecs() {
    return pimpl->getAudioSpecs();
}

Track::operator bool() {
    return bool(*pimpl);
}

} //end namespace Melosic
