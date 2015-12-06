/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#include <openssl/md5.h>

#include "flacdecoder.hpp"
#include "flac_provider.hpp"

namespace flac {

bool provider::supports_mime(std::string_view mime_type) const {
    return mime_type == "audio/flac" || mime_type == "audio/x-flac";
}

std::unique_ptr<Melosic::Decoder::PCMSource> provider::make_decoder(std::unique_ptr<std::istream> in) const {
    return std::make_unique<FlacDecoder>(std::move(in));
}

bool provider::verify(std::unique_ptr<std::istream> in) const {
    return false;
}

} // namespace flac
