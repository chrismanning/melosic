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
    impl(IO::BiDirectionalClosableSeekable& input, Input::Factory factory) :
        impl(input, factory, std::chrono::milliseconds(0)) {}
    impl(IO::BiDirectionalClosableSeekable& input, Input::Factory factory, std::chrono::milliseconds offset) :
        input(input),
        factory(factory),
        offset(offset)
    {
        input.seek(0, std::ios_base::beg, std::ios_base::in);
        decoder = factory(input);
        input.close();
    }

    ~impl() {}

    std::streamsize read(char * s, std::streamsize n) {
        if(!input.isOpen()) {
            input.reOpen();
            decoder->seek(offset);
        }
        return decoder->read(s, n);
    }

    std::streampos seek(std::streamoff off,
                        std::ios_base::seekdir way,
                        std::ios_base::openmode which)
    {
        if(!input.isOpen()) {
            input.reOpen();
            decoder->seek(offset);
        }
        return decoder->seekg(off, way);
    }

    Track::TagsType& getTags() {
        return tags;
    }

    void seek(std::chrono::milliseconds dur) {
        if(!input.isOpen()) {
            input.reOpen();
            decoder->seek(offset);
        }
        decoder->seek(dur + offset);
    }

    std::chrono::milliseconds tell() {
        if(!input.isOpen()) {
            input.reOpen();
            decoder->seek(offset);
        }
        auto pos = input.seek(0, std::ios_base::cur, std::ios_base::in);

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
    IO::BiDirectionalClosableSeekable& input;
    Input::Factory factory;
    std::chrono::milliseconds offset;
    std::shared_ptr<Input::ISource> decoder;
    std::once_flag decoderInitFlag;
    Track::TagsType tags;
};

Track::Track(IO::BiDirectionalClosableSeekable& input, Input::Factory factory) :
    Track(input, factory, std::chrono::milliseconds(0)) {}
Track::Track(IO::BiDirectionalClosableSeekable& input, Input::Factory factory, std::chrono::milliseconds offset) :
    pimpl(new impl(input, factory, offset)) {}

Track::~Track() {}

std::streamsize Track::do_read(char * s, std::streamsize n) {
    return pimpl->read(s, n);
}

std::streampos Track::do_seekg(std::streamoff off, std::ios_base::seekdir way) {
    return pimpl->seek(off, way, std::ios_base::in);
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
