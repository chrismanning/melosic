/**************************************************************************
**  Copyright (C) 2016 Christian Manning
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

#include <vector>
#include <string>
using namespace std::literals;

#include <alsa/asoundlib.h>

#include <melosic/common/error.hpp>
#include <melosic/asio/audio_io.hpp>

#include "alsa.hpp"
#include "alsa_provider.hpp"
#include "alsa_output_service_impl.hpp"

namespace alsa {

typedef Melosic::AudioIO::AudioOutputService<output_service_impl> AlsaOutputService;
typedef Melosic::AudioIO::BasicAudioOutput<AlsaOutputService> AlsaAsioOutput;

std::vector<Melosic::Output::device_descriptor> provider::available_devices() const {
    const auto predicate = [](auto* str) { return str == nullptr || "Output"s == str; };

    void** hints;
    ALSA_THROW_IF(Melosic::DeviceOpenException, snd_device_name_hint(-1, "pcm", &hints));

    std::vector<Melosic::Output::device_descriptor> names;

    TRACE_LOG(logject) << "Enumerating output devices";
    for(auto n = hints; *n != nullptr; n++) {
        auto name = snd_device_name_get_hint(*n, "NAME");
        auto desc = snd_device_name_get_hint(*n, "DESC");
        auto io = snd_device_name_get_hint(*n, "IOID");
        if(predicate(io) && name && desc) {
            names.emplace_back(name, desc);
            TRACE_LOG(logject) << names.back();
        }
        free(name);
        free(desc);
        free(io);
    }
    snd_device_name_free_hint(hints);

    return names;
}

std::string provider::device_prefix() const {
    return "ALSA";
}

std::unique_ptr<Melosic::AudioIO::AudioOutputBase>
provider::make_output(asio::io_service& io_service, const Melosic::Output::device_descriptor& desc) const {
}

} // namespace alsa
