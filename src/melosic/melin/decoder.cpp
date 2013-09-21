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

#include <boost/range/adaptor/map.hpp>
using namespace boost::adaptors;
#include <boost/algorithm/string/case_conv.hpp>

#include <taglib/aifffile.h>
#include <taglib/mpcfile.h>
#include <taglib/tfile.h>
#include <taglib/apefile.h>
#include <taglib/mpegfile.h>
#include <taglib/trueaudiofile.h>
#include <taglib/asffile.h>
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/flacfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/wavfile.h>
#include <taglib/itfile.h>
#include <taglib/rifffile.h>
#include <taglib/wavpackfile.h>
#include <taglib/modfile.h>
#include <taglib/s3mfile.h>
#include <taglib/xmfile.h>
#include <taglib/mp4file.h>
#include <taglib/speexfile.h>
#include <taglib/opusfile.h>

#include <melosic/melin/logging.hpp>
#include <melosic/common/file.hpp>
#include <melosic/common/string.hpp>
#include <melosic/core/taglibfile.hpp>

#include "decoder.hpp"

namespace Melosic {
namespace Decoder {

using namespace TagLib;

Logger::Logger logject{logging::keywords::channel = "Decoder::Manager"};

class Manager::impl {
public:
    impl() = default;

    void addAudioFormat(Factory fact, std::string extension) {
        boost::to_lower(extension);
        LOG(logject) << "Adding support for extension " << extension;
        auto bef = inputFactories.size();
        auto pos = inputFactories.find(extension);
        if(pos == inputFactories.end()) {
            inputFactories.emplace(std::move(extension), fact);
            assert(++bef == inputFactories.size());
        }
        else {
            WARN_LOG(logject) << extension << ": already registered to a decoder factory";
        }
    }

private:
    std::unordered_map<std::string, Factory> inputFactories;
    friend struct FileTypeResolver;
};

Manager::Manager() : pimpl(new impl) {}

Manager::~Manager() {}

void Manager::addAudioFormat(Factory fact, std::string extension) {
    pimpl->addAudioFormat(fact, extension);
}

decltype(FileTypeResolver::chan) FileTypeResolver::chan;

FileTypeResolver Manager::getFileTypeResolver(const boost::filesystem::path& path) const {
    if(!path.has_extension())
        BOOST_THROW_EXCEPTION(UnsupportedTypeException() << ErrorTag::FilePath(path));
    return FileTypeResolver(*this, path.extension().string());
}

FileTypeResolver::FileTypeResolver(const Manager& decman, std::string ext) {
    CHAN_TRACE_LOG(logject, chan) << "Trying to open file with extension " << ext;
    auto& inputFactories = decman.pimpl->inputFactories;
    boost::to_lower(ext);
    CHAN_TRACE_LOG(logject, chan) << "lowered extension: " << ext;
    auto fact = inputFactories.find(ext);

    if(fact != inputFactories.end())
        decoderFactory = fact->second;
    else {
        CHAN_WARN_LOG(logject, chan) << "Cannot open file with extension " << ext;
        CHAN_WARN_LOG(logject, chan) << "An exception will be thrown when attempting to open";
        decoderFactory = [=](Factory::argument_type a) -> Factory::result_type {
            BOOST_THROW_EXCEPTION(UnsupportedTypeException() << ErrorTag::FileExtension(ext));
        };
    }

    if(ext == ".mp3")
        tagFactory = [] (IOStream* a) { return new MPEG::File(a, nullptr); };
    else if(ext == ".ogg")
        tagFactory = [] (IOStream* a) { return new Ogg::Vorbis::File(a); };
    else if(ext == ".oga")
      /* .oga can be any audio in the Ogg container. First try FLAC, then Vorbis. */
        tagFactory = [] (IOStream* a) {
            TagLib::File* ptr = new Ogg::FLAC::File(a);
            return ptr->isValid() ? ptr : new Ogg::Vorbis::File(a);
        };
    else if(ext == ".flac")
        tagFactory = [] (IOStream* a) { return new FLAC::File(a, nullptr); };
    else if(ext == ".mpc")
        tagFactory = [] (IOStream* a) { return new MPC::File(a); };
    else if(ext == ".wv")
        tagFactory = [] (IOStream* a) { return new WavPack::File(a); };
    else if(ext == ".spx")
        tagFactory = [] (IOStream* a) { return new Ogg::Speex::File(a); };
    else if(ext == ".opus")
        tagFactory = [] (IOStream* a) { return new Ogg::Opus::File(a); };
    else if(ext == ".tta")
        tagFactory = [] (IOStream* a) { return new TrueAudio::File(a); };
    else if(ext == ".m4a" || ext == ".m4r" || ext == ".m4b" || ext == ".m4p" || ext == ".mp4" || ext == ".3g2")
        tagFactory = [] (IOStream* a) { return new MP4::File(a); };
    else if(ext == ".wma" || ext == ".asf")
        tagFactory = [] (IOStream* a) { return new ASF::File(a); };
    else if(ext == ".aif" || ext == ".aiff" || ext == ".afc" || ext == ".aifc")
        tagFactory = [] (IOStream* a) { return new RIFF::AIFF::File(a); };
    else if(ext == ".wav")
        tagFactory = [] (IOStream* a) { return new RIFF::WAV::File(a); };
    else if(ext == ".ape")
        tagFactory = [] (IOStream* a) { return new APE::File(a); };
    // module, nst and wow are possible but uncommon extensions
    else if(ext == ".mod" || ext == ".module" || ext == ".nst" || ext == ".wow")
        tagFactory = [] (IOStream* a) { return new Mod::File(a); };
    else if(ext == ".s3m")
        tagFactory = [] (IOStream* a) { return new S3M::File(a); };
    else if(ext == ".it")
        tagFactory = [] (IOStream* a) { return new IT::File(a); };
    else if(ext == ".xm")
        tagFactory = [] (IOStream* a) { return new XM::File(a); };
    else {
        CHAN_WARN_LOG(logject, chan) << "Cannot open file with extension " << ext;
        CHAN_WARN_LOG(logject, chan) << "An exception will be thrown when attempting to read tags";
        tagFactory = [=](IOStream*) -> File* {
            BOOST_THROW_EXCEPTION(MetadataUnsupportedException() <<
                                  ErrorTag::FileExtension(ext));
        };
    }
}

std::unique_ptr<Decoder::Playable> FileTypeResolver::getDecoder(IO::File& file) {
    return decoderFactory(file);
}

std::unique_ptr<File> FileTypeResolver::getTagReader(IO::File& file) {
    taglibFile.reset(new IO::TagLibFile(file));
    return std::unique_ptr<File>(tagFactory(taglibFile.get()));
}

} // namespace Decoder
} // namespace Melosic
