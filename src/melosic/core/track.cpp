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
using Mutex = boost::shared_mutex;
using shared_lock_guard = boost::shared_lock_guard<Mutex>;
#include <thread>
#include <mutex>
using unique_lock = std::unique_lock<Mutex>;
using lock_guard = std::lock_guard<Mutex>;

#include <taglib/tpropertymap.h>
#include <taglib/tfile.h>

#include "track.hpp"
#include <melosic/common/stream.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/file.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/decoder.hpp>

namespace Melosic {
namespace Core {

static Logger::Logger logject(logging::keywords::channel = "Track");

class Track::impl {
public:
    impl(Decoder::Manager& decman,
         boost::filesystem::path filepath,
         chrono::milliseconds start,
         chrono::milliseconds end)
        : input(new IO::File(filepath)),
          start(start),
          end(end),
          fileResolver(decman.getFileTypeResolver(filepath)),
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
        return input->isOpen() && static_cast<bool>(decoder);
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
        decoder->seek(dur + start);
    }

    chrono::milliseconds tell() {
        if(!isOpen()) {
            reOpen();
        }
        return decoder->tell() - start;
    }

    void reset() {
        if(isOpen()) {
            decoder->reset();
        }
    }

    chrono::milliseconds duration() {
        if(end > start) {
            return end - start;
        }

        return chrono::milliseconds(uint64_t(as.total_samples / (as.sample_rate/1000.0)));
    }

    AudioSpecs& getAudioSpecs() {
        return as;
    }

    std::string getTag(const std::string& key) {
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
        taglibfile = fileResolver.getTagReader(*input);
        tags = taglibfile->properties();
        seek(pos);
        TRACE_LOG(logject) << tags.toString().to8Bit(true);
    }

    bool valid() {
        if(decoder) {
            return bool(*decoder);
        }
        else
            return false;
    }

    std::string sourceName() {
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
    Mutex mu;
};

Track::Track(Decoder::Manager& decman,
             const boost::filesystem::path& filename,
             chrono::milliseconds start,
             chrono::milliseconds end)
    : pimpl(new impl(decman, filename, start, end)) {}

Track::~Track() {}

Track::Track(const Track& b) : pimpl(b.pimpl->clone()) {}

Track::Track(Track&&) = default;

Track& Track::operator=(Track b) {
    shared_lock_guard l1(pimpl->mu);
    shared_lock_guard l2(b.pimpl->mu);
    using std::swap;
    swap(pimpl, b.pimpl);
    return *this;
}

bool Track::operator==(const Track& b) const {
    shared_lock_guard l1(pimpl->mu);
    shared_lock_guard l2(b.pimpl->mu);
    return pimpl == b.pimpl;
}

std::streamsize Track::read(char * s, std::streamsize n) {
    lock_guard l(pimpl->mu);
    return pimpl->read(s, n);
}

void Track::close() {
    lock_guard l(pimpl->mu);
    pimpl->close();
}

bool Track::isOpen() const {
    shared_lock_guard l(pimpl->mu);
    return pimpl->isOpen();
}

void Track::reOpen() {
    lock_guard l(pimpl->mu);
    pimpl->reOpen();
}

void Track::seek(chrono::milliseconds dur) {
    lock_guard l(pimpl->mu);
    pimpl->seek(dur);
}

chrono::milliseconds Track::tell() {
    shared_lock_guard l(pimpl->mu);
    return pimpl->tell();
}

void Track::reset() {
    lock_guard l(pimpl->mu);
    pimpl->reset();
}

chrono::milliseconds Track::duration() const {
    shared_lock_guard l(pimpl->mu);
    return pimpl->duration();
}

Melosic::AudioSpecs& Track::getAudioSpecs() {
    shared_lock_guard l(pimpl->mu);
    return pimpl->getAudioSpecs();
}
const Melosic::AudioSpecs& Track::getAudioSpecs() const {
    shared_lock_guard l(pimpl->mu);
    return pimpl->getAudioSpecs();
}

std::string Track::getTag(const std::string& key) const {
    shared_lock_guard l(pimpl->mu);
    return pimpl->getTag(key);
}

Track::operator bool() {
    shared_lock_guard l(pimpl->mu);
    return pimpl->valid();
}

std::string Track::sourceName() const {
    shared_lock_guard l(pimpl->mu);
    return pimpl->sourceName();
}

void Track::reloadTags() {
    unique_lock l(pimpl->mu);
    pimpl->reloadTags();
}

void Track::reloadDecoder() {
    lock_guard l(pimpl->mu);
    pimpl->reloadDecoder();
}

} // namespace Core
} // namespace Melosic
