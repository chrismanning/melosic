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

#include <map>
#include <string>

#include <boost/range/adaptor/map.hpp>
using namespace boost::adaptors;

#include <taglib/flacfile.h>

#include <melosic/melin/logging.hpp>
#include <melosic/common/file.hpp>
#include <melosic/core/taglibfile.hpp>

#include "decoder.hpp"

template class std::function<std::unique_ptr<Melosic::Decoder::Playable>(Melosic::IO::BiDirectionalClosableSeekable&)>;

namespace Melosic {
namespace Decoder {

class Manager::impl {
public:
    impl() {}

    void addAudioFormat(Factory fact, const std::string& extension) {
        auto bef = inputFactories.size();
        auto pos = inputFactories.find(extension);
        if(pos == inputFactories.end()) {
            inputFactories.emplace(extension, fact);
            assert(++bef == inputFactories.size());
        }
        else {
            WARN_LOG(logject) << extension << ": already registered to a decoder factory";
        }
    }

private:
    std::map<std::string, Factory> inputFactories;
    Logger::Logger logject{logging::keywords::channel = "Decoder::Manager"};
    friend struct FileTypeResolver;
};

Manager::Manager() : pimpl(new impl) {}

Manager::~Manager() {}

void Manager::addAudioFormat(Factory fact, const std::string& extension) {
    pimpl->addAudioFormat(fact, extension);
}

FileTypeResolver::FileTypeResolver(Manager& decman, const boost::filesystem::path& filepath) {
    auto ext = filepath.extension().string();

    auto& inputFactories = decman.pimpl->inputFactories;
    auto fact = inputFactories.find(ext);

    if(fact != inputFactories.end()) {
        decoderFactory = fact->second;
    }
    else {
        std::clog << "Supported file types (" << inputFactories.size() << "): ";
        for(const auto& key : inputFactories | map_keys) {
            std::clog << key << ", ";
        }
        std::clog << std::endl;
        decoderFactory = [](decltype(decoderFactory)::argument_type a) -> decltype(decoderFactory)::result_type {
            try {
                IO::File& b = dynamic_cast<IO::File&>(a);
                BOOST_THROW_EXCEPTION(UnsupportedFileTypeException() <<
                                      ErrorTag::FilePath(absolute(b.filename())));
            }
            catch(std::bad_cast& e) {
                BOOST_THROW_EXCEPTION(UnsupportedTypeException());
            }
        };
    }

    if(ext == ".flac") {
        tagFactory = std::bind([](TagLib::IOStream* a, TagLib::ID3v2::FrameFactory* b) {
                                    return new TagLib::FLAC::File(a, b);
                               },
                               std::placeholders::_1,
                               nullptr);
    }
    else {
        tagFactory = [=](TagLib::IOStream*) -> TagLib::File* {
            BOOST_THROW_EXCEPTION(MetadataUnsupportedException() <<
                                  ErrorTag::FilePath(absolute(filepath)));
        };
    }
}

std::unique_ptr<Decoder::Playable> FileTypeResolver::getDecoder(IO::File& file) {
    return decoderFactory(file);
}

std::unique_ptr<TagLib::File> FileTypeResolver::getTagReader(IO::File& file) {
    taglibFile.reset(new IO::TagLibFile(file));
    return std::unique_ptr<TagLib::File>(tagFactory(taglibFile.get()));
}

} // namespace Decoder
} // namespace Melosic
