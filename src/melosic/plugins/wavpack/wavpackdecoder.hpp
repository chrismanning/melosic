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

#ifndef WAVPACKDECODER_HPP
#define WAVPACKDECODER_HPP

#include <vector>

#include <melosic/melin/decoder.hpp>
#include <melosic/melin/exports.hpp>
using namespace Melosic;

#include "./exports.hpp"

#include <wavpack/wavpack.h>

namespace wavpack {

extern Plugin::Info wavpack_info;

struct WAVPACK_MELIN_API wavpack_decoder : Decoder::PCMSource {
    explicit wavpack_decoder(std::unique_ptr<std::istream> input);

    virtual ~wavpack_decoder();

    void seek(chrono::milliseconds dur);
    chrono::milliseconds tell() const;
    chrono::milliseconds duration() const;
    AudioSpecs getAudioSpecs() const;
    size_t decode(PCMBuffer& buf, std::error_code& ec);
    bool valid() const;
    void reset();

    struct WavpackDestroyer {
        void operator()(WavpackContext* ptr) {
            WavpackCloseFile(ptr);
        }
    };

    AudioSpecs as;
    std::vector<char> buf;
    std::unique_ptr<std::istream> m_input;
    ::WavpackStreamReader m_stream_reader;
    std::unique_ptr<::WavpackContext, WavpackDestroyer> m_wavpack;
};

} // namespace wavpack

#endif // WAVPACKDECODER_HPP
