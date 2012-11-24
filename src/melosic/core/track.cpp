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

#include <taglib/tpropertymap.h>
#include <taglib/tfile.h>

#include "track.hpp"
#include <melosic/common/stream.hpp>
#include <melosic/managers/input/pluginterface.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>

namespace Melosic {

static Logger::Logger logject(boost::log::keywords::channel = "Track");

//TODO: tracks need a length to support multiple tracks per file
class Track::impl {
public:
    impl(std::string filename, std::chrono::milliseconds start, std::chrono::milliseconds end) :
        filename(filename), input(new IO::File(filename)), start(start), end(end), fileResolver(*input)
    {
        input->seek(0, std::ios_base::beg, std::ios_base::in);
        decoder = fileResolver.getDecoder();
        reloadTags();
        close();
    }

    ~impl() {}

    bool isOpen() {
        return input->isOpen();
    }

    void reOpen() {
        if(!isOpen()) {
            input->reOpen();
        }
        reset();
        std::lock_guard<Mutex> l(mu);
        decoder->seek(start);
    }

    void close() {
        input->close();
    }

    std::streamsize read(char * s, std::streamsize n) {
        if(!isOpen()) {
            reOpen();
        }
        auto difference = (end > start) ? (end - tell()).count() : 0;
        std::lock_guard<Mutex> l(mu);
        return decoder->read(s, (difference < n) ? n : difference);
    }

    void seek(std::chrono::milliseconds dur) {
        if(!isOpen()) {
            reOpen();
        }
        std::lock_guard<Mutex> l(mu);
        decoder->seek(dur + start);
    }

    std::chrono::milliseconds tell() {
        if(!isOpen()) {
            reOpen();
        }
        boost::shared_lock<Mutex> l(mu);
        return decoder->tell() - start;
    }

    void reset() {
        if(!isOpen()) {
            reOpen();
        }
        std::lock_guard<Mutex> l(mu);
        decoder->reset();
    }

    std::chrono::milliseconds duration() const {
        auto& as = const_cast<impl*>(this)->getAudioSpecs();
        return std::chrono::milliseconds(uint64_t(as.total_samples / (as.sample_rate/1000.0)));
    }

    AudioSpecs& getAudioSpecs() {
        boost::shared_lock<Mutex> l(mu);
        return decoder->getAudioSpecs();
    }

    std::string getTag(const std::string& key) const {
        auto val = tags.find(key);

        if(val != tags.end()) {
            return val->second.front().to8Bit(true);
        }
        static std::string str("?");
        return str;
    }

    void reloadTags() {
        taglibfile = fileResolver.getTagReader();
        tags = taglibfile->properties();
        TRACE_LOG(logject) << tags.toString().to8Bit(true);
    }

    explicit operator bool() {
        boost::shared_lock<Mutex> l(mu);
        if(decoder) {
            return bool(*decoder);
        }
        else
            return false;
    }

    const std::string& sourceName() const {
        return input->filename().string();
    }

private:
    friend class Track;
    const std::string& filename;
    std::unique_ptr<IO::File> input;
    std::chrono::milliseconds start, end;
    Kernel::FileTypeResolver fileResolver;
    std::unique_ptr<Input::Source> decoder;
    std::unique_ptr<TagLib::File> taglibfile;
    TagLib::PropertyMap tags;
    typedef boost::shared_mutex Mutex;
    Mutex mu;
};

Track::Track(const std::string& filename, std::chrono::milliseconds start, std::chrono::milliseconds end) :
    pimpl(new impl(filename, start, end)) {}

Track::~Track() {}

Track::Track(const Track& b) {
    auto filename = b.sourceName();
    TRACE_LOG(logject) << "Copying " << filename;
    pimpl.reset(new impl(filename, b.pimpl->start, b.pimpl->end));
}

void Track::operator=(const Track& b) {
    auto filename = b.sourceName();
    TRACE_LOG(logject) << "Assigning " << filename;
    pimpl.reset(new impl(filename, b.pimpl->start, b.pimpl->end));
}

std::streamsize Track::read(char * s, std::streamsize n) {
    return pimpl->read(s, n);
}

void Track::close() {
    pimpl->close();
}

bool Track::isOpen() {
    return pimpl->isOpen();
}

void Track::reOpen() {
    pimpl->reOpen();
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

std::chrono::milliseconds Track::duration() const {
    return pimpl->duration();
}

Melosic::AudioSpecs& Track::getAudioSpecs() {
    return pimpl->getAudioSpecs();
}

std::string Track::getTag(const std::string& key) const {
    return pimpl->getTag(key);
}

Track::operator bool() {
    return bool(*pimpl);
}

const std::string& Track::sourceName() const {
    return pimpl->sourceName();
}

} //end namespace Melosic
