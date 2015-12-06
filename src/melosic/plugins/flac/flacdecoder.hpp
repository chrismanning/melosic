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

#ifndef FLACDECODER_HPP
#define FLACDECODER_HPP

#include <memory>
#include <istream>
#include <deque>
#include <algorithm>
#include <cmath>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/common/pcmbuffer.hpp>

#include "./exports.hpp"

using namespace Melosic;

extern const Plugin::Info flacInfo;

class FLAC_MELIN_API FlacDecoder : public Decoder::PCMSource {
  public:
    explicit FlacDecoder(std::unique_ptr<std::istream> input);

    virtual ~FlacDecoder();

    size_t decode(PCMBuffer& pcm_buf, std::error_code& ec) override;
    void seek(chrono::milliseconds dur) override;
    chrono::milliseconds tell() const override;
    chrono::milliseconds duration() const override;
    void reset() override;
    AudioSpecs getAudioSpecs() const override;
    bool valid() const override;

  private:
    AudioSpecs as;
    std::deque<char> buf;
    struct FlacDecoderImpl;
    std::unique_ptr<FlacDecoderImpl> m_decoder;
};

#endif // FLACDECODER_HPP
