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

#include <iostream>
#include <array>

#include <openssl/md5.h>

#include "wavpack_provider.hpp"
#include "wavpackdecoder.hpp"

namespace wavpack {

bool provider::supports_mime(std::string_view mime_type) const {
    return mime_type == "audio/x-wavpack";
}

std::unique_ptr<Melosic::Decoder::PCMSource> provider::make_decoder(std::unique_ptr<std::istream> in) const {
    return std::make_unique<wavpack_decoder>(std::move(in));
}

bool provider::verify(std::unique_ptr<std::istream> in) const {
    auto decoder = std::make_unique<wavpack_decoder>(std::move(in));
    std::array<unsigned char, MD5_DIGEST_LENGTH> expected_md5{{0}};
    WavpackGetMD5Sum(decoder->m_wavpack.get(), expected_md5.data());

    return get_pcm_md5(std::move(decoder)) == expected_md5;
}

} // namespace wavpack
