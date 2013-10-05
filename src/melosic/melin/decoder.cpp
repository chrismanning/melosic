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
#include <boost/scope_exit.hpp>

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
#include <melosic/core/track.hpp>

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

    std::optional<Core::Track> openTrack(boost::filesystem::path,
                                         chrono::milliseconds,
                                         chrono::milliseconds) noexcept;

private:
    std::unordered_map<std::string, Factory> inputFactories;
};

Manager::Manager() : pimpl(new impl) {}

Manager::~Manager() {}

void Manager::addAudioFormat(Factory fact, std::string extension) {
    pimpl->addAudioFormat(fact, extension);
}

std::optional<Core::Track> Manager::impl::openTrack(boost::filesystem::path filepath,
                                     chrono::milliseconds start,
                                     chrono::milliseconds end) noexcept
{
    auto ext = filepath.extension().string();
    if(ext.empty())
        return {};
    boost::to_lower(ext);
    TRACE_LOG(logject) << "Trying to open file with extension " << ext;
    auto fact = inputFactories.find(ext);

    Factory decoderFactory;
    TRACE_LOG(logject) << "Factory decoderFactory;";
    if(fact != inputFactories.end()) {
        TRACE_LOG(logject) << "fact != inputFactories.end()";
        decoderFactory = fact->second;
    }
    else {
        ERROR_LOG(logject) << "Cannot decode file with extension " << ext;
        decoderFactory = [=](Factory::argument_type) -> Factory::result_type {
            BOOST_THROW_EXCEPTION(UnsupportedTypeException() << ErrorTag::FileExtension(ext));
        };
    }

    Core::Track t{std::move(decoderFactory), std::move(filepath), start, end};

    return t;
}
std::optional<Core::Track> Manager::openTrack(boost::filesystem::path filepath,
                                                chrono::milliseconds start,
                                                chrono::milliseconds end) const noexcept
{
    return pimpl->openTrack(std::move(filepath), start, end);
}

} // namespace Decoder
} // namespace Melosic
