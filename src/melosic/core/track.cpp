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

#include <boost/thread.hpp>
#include <boost/thread/shared_lock_guard.hpp>
using boost::shared_mutex; using boost::shared_lock_guard;
#include <thread>
#include <mutex>
using std::unique_lock; using std::lock_guard;

#include <taglib/tpropertymap.h>
#include <taglib/tfile.h>

#include "track.hpp"
#include <melosic/common/stream.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/decoder.hpp>

namespace Melosic {
namespace Core {

static Logger::Logger logject(logging::keywords::channel = "Track");

//TODO: tracks need a length to support multiple tracks per file
class Track::impl {
public:
    impl(Decoder::Manager& decman,
         boost::filesystem::path filepath,
         chrono::milliseconds start,
         chrono::milliseconds end)
        : input(new IO::File(filepath)),
          start(start),
          end(end),
          fileResolver(decman, filepath),
          decman(decman)
    {
        input->seek(0, std::ios_base::beg, std::ios_base::in);
        reloadDecoder();
        reloadTags();
        taglibfile.reset();
        close();
    }

    ~impl() {}

    bool isOpen() {
        return input->isOpen() && bool(decoder);
    }

    void reOpen() {
        if(!input->isOpen()) {
            input->reOpen();
        }
        assert(input->isOpen());
        reloadDecoder();
        seek(start-start);
    }

    void close() {
        decoder.reset();
        input->close();
    }

    std::streamsize read(char * s, std::streamsize n) {
        if(!isOpen()) {
            reOpen();
        }
        try {
            auto difference = (end > start) ? (end - tell()).count() : 0;
            lock_guard<Mutex> l(mu);
            return decoder->read(s, (difference < n) ? n : difference);
        }
        catch(AudioDataInvalidException& e) {
            e << ErrorTag::FilePath(sourceName());
            throw;
        }
    }

    void seek(chrono::milliseconds dur) {
        if(!isOpen()) {
            reOpen();
        }
        lock_guard<Mutex> l(mu);
        decoder->seek(dur + start);
    }

    chrono::milliseconds tell() {
        if(!isOpen()) {
            reOpen();
        }
        shared_lock_guard<Mutex> l(mu);
        return decoder->tell() - start;
    }

    void reset() {
        if(isOpen()) {
            lock_guard<Mutex> l(mu);
            decoder->reset();
        }
    }

    chrono::milliseconds duration() const {
        if(end > start) {
            return end - start;
        }

        return chrono::milliseconds(uint64_t(as.total_samples / (as.sample_rate/1000.0)));
    }

    AudioSpecs& getAudioSpecs() {
        return as;
    }
    const AudioSpecs& getAudioSpecs() const {
        return as;
    }

    std::string getTag(const std::string& key) const {
        TagLib::String key_(key.c_str());
        auto val = tags.find(key_);

        if(val != tags.end()) {
            return val->second.front().to8Bit(true);
        }
        static std::string str("?");
        return str;
    }

    void reloadDecoder() {
        try {
            lock_guard<Mutex> l(mu);
            decoder = fileResolver.getDecoder(*input);
            as = decoder->getAudioSpecs();
        }
        catch(DecoderException& e) {
            e << ErrorTag::FilePath(input->filename());
            throw;
        }
    }

    void reloadTags() {
        if(!isOpen()) {
            reOpen();
        }
        auto pos = tell();
        boost::unique_lock<Mutex> l(mu);
        taglibfile = fileResolver.getTagReader(*input);
        tags = taglibfile->properties();
        l.unlock();
        seek(pos);
        TRACE_LOG(logject) << tags.toString().to8Bit(true);
    }

    explicit operator bool() {
        shared_lock_guard<Mutex> l(mu);
        if(decoder) {
            return bool(*decoder);
        }
        else
            return false;
    }

    const std::string& sourceName() const {
        return input->filename().string();
    }

    impl* clone() {
        return new impl(decman, input->filename(), start, end);
    }

private:
    friend class Track;
    std::unique_ptr<IO::File> input;
    chrono::milliseconds start, end;
    Decoder::FileTypeResolver fileResolver;
    std::unique_ptr<Decoder::Playable> decoder;
    std::unique_ptr<TagLib::File> taglibfile;
    Decoder::Manager& decman;
    TagLib::PropertyMap tags;
    AudioSpecs as;
    typedef shared_mutex Mutex;
    Mutex mu;
};

Track::Track(Decoder::Manager& decman,
             const boost::filesystem::path& filename,
             chrono::milliseconds start,
             chrono::milliseconds end)
    : pimpl(new impl(decman, filename, start, end)) {}

Track::~Track() {}

Track::Track(const Track& b) {
    auto filename = b.sourceName();
    TRACE_LOG(logject) << "Copying " << filename;
    pimpl.reset(b.pimpl->clone());
}

Track& Track::operator=(const Track& b) {
    auto filename = b.sourceName();
    TRACE_LOG(logject) << "Assigning " << filename;
    pimpl.reset(b.pimpl->clone());
    return *this;
}

bool Track::operator==(const Track& b) const {
    return pimpl == b.pimpl;
}

std::streamsize Track::read(char * s, std::streamsize n) {
    return pimpl->read(s, n);
}

void Track::close() {
    pimpl->close();
}

bool Track::isOpen() const {
    return pimpl->isOpen();
}

void Track::reOpen() {
    pimpl->reOpen();
}

void Track::seek(chrono::milliseconds dur) {
    pimpl->seek(dur);
}

chrono::milliseconds Track::tell() {
    return pimpl->tell();
}

void Track::reset() {
    pimpl->reset();
}

chrono::milliseconds Track::duration() const {
    return pimpl->duration();
}

Melosic::AudioSpecs& Track::getAudioSpecs() {
    return pimpl->getAudioSpecs();
}
const AudioSpecs& Track::getAudioSpecs() const {
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

void Track::reloadTags() {
    pimpl->reloadTags();
}

void Track::reloadDecoder() {
    pimpl->reloadDecoder();
}

} // namespace Core
} // namespace Melosic
