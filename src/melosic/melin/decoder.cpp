/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#include <unordered_map>
#include <string>
#include <functional>
namespace ph = std::placeholders;
#include <mutex>
using mutex = std::mutex;
using unique_lock = std::unique_lock<mutex>;

#include <boost/range/adaptor/map.hpp>
using namespace boost::adaptors;
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/filesystem/fstream.hpp>

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

#include <melosic/common/optional.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/string.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/filecache.hpp>
#include <melosic/core/audiofile.hpp>
#include <melosic/common/thread.hpp>

#include "decoder.hpp"

namespace Melosic {
namespace Decoder {

using namespace TagLib;

Logger::Logger logject{logging::keywords::channel = "Decoder::Manager"};

class Manager::impl {
public:
    impl(Thread::Manager& tman) : tman(tman) {}
    Thread::Manager& tman;
    mutex mu;
    std::unordered_map<std::string, Factory> inputFactories;
    Core::FileCache m_file_cache;
};

Manager::Manager(Thread::Manager& tman) : pimpl(new impl(tman)) {}

Manager::~Manager() {}

void Manager::addAudioFormat(Factory fact, std::string extension) {
    unique_lock l(pimpl->mu);
    boost::to_lower(extension);
    LOG(logject) << "Adding support for extension " << extension;
    auto bef = pimpl->inputFactories.size();
    auto pos = pimpl->inputFactories.find(extension);
    if(pos == pimpl->inputFactories.end()) {
        pimpl->inputFactories.emplace(std::move(extension), fact);
        assert(++bef == pimpl->inputFactories.size());
    }
    else {
        WARN_LOG(logject) << extension << ": already registered to a decoder factory";
    }
}

optional<Core::AudioFile> Manager::getFile(boost::filesystem::path p) const {
    boost::system::error_code ec;
    return pimpl->m_file_cache.getFile(p, ec);
}

std::future<bool> Manager::initialiseAudioFile(Core::AudioFile& af) const {
    return pimpl->tman.enqueue([&af, this] () mutable {
        FileRef taglib_file{af.filePath().c_str()};
        if(taglib_file.isNull())
            return false;
        assert(taglib_file.tag() != nullptr);
        auto taglib_tags = taglib_file.tag()->properties();
        if(/*is playlist or cue*/taglib_tags.contains("CUEFILE"))
            return false;
        Core::Track t{af.filePath()};
        Core::TagMap tags;
        for(const auto& tag : taglib_tags) {
            for(const auto& v : tag.second)
                tags.emplace(tag.first.to8Bit(), v.to8Bit());
        }
        t.setTags(tags);
        auto pcm_src = openTrack(t);
        if(pcm_src) {
            t.setEnd(pcm_src->duration());
            t.setAudioSpecs(pcm_src->getAudioSpecs());
        }
        else {
            auto ap = taglib_file.audioProperties();
            t.setAudioSpecs({(uint8_t)ap->channels(), 0, (uint32_t)ap->sampleRate()});
            t.setEnd(chrono::seconds{ap->length()});
        }
        af.tracks()->push_back(t);
        return true;
    });
}

std::vector<Core::Track> Manager::openPath(boost::filesystem::path p) const {
    auto af = getFile(std::move(p));
    assert(af);
    if(af->trackCount() == 0) {
        auto fut = initialiseAudioFile(*af);
        if(!fut.get())
            return {};
    }
    return af->tracks().get();
}

std::unique_ptr<PCMSource> Manager::openTrack(const Core::Track& track) const {
    unique_lock l(pimpl->mu);
    auto ext = track.filePath().extension().string();
    if(ext.empty())
        return nullptr;
    boost::to_lower(ext);
    TRACE_LOG(logject) << "Trying to open file with extension " << ext;
    auto fact = pimpl->inputFactories.find(ext);
    if(fact != pimpl->inputFactories.end())
        return fact->second(std::make_unique<fs::ifstream>(track.filePath()));
    return nullptr;
}

} // namespace Decoder
} // namespace Melosic
