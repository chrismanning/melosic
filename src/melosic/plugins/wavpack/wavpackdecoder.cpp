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

#include <boost/integer.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/seek.hpp>
namespace io = boost::iostreams;
#include <asio/error.hpp>

#include <melosic/melin/logging.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/common/pcmbuffer.hpp>
#include <melosic/common/error.hpp>
#include <melosic/common/pcm_sample.hpp>
using namespace Melosic;

#include "wavpackdecoder.hpp"

static Logger::Logger logject{logging::keywords::channel = "Wavpack"};

int32_t read_bytes_impl(void* input, void* data, int32_t bcount) {
    try {
        return io::read(*static_cast<std::istream*>(input), static_cast<char*>(data), bcount);
    } catch(std::ios_base::failure& e) {
        ERROR_LOG(logject) << e.what();
        return 0;
    }
}

uint32_t get_pos_impl(void* input) {
    return io::position_to_offset(io::seek(*static_cast<std::istream*>(input), 0, std::ios_base::cur));
}

int set_pos_abs_impl(void* input, uint32_t pos) {
    io::seek(*static_cast<std::istream*>(input), pos, std::ios_base::beg);
    return 0;
}

int set_pos_rel_impl(void* input, int32_t delta, int mode) {
    std::ios_base::seekdir seek_dir;
    switch(mode) {
        case SEEK_SET:
            seek_dir = std::ios_base::beg;
            break;
        case SEEK_CUR:
            seek_dir = std::ios_base::cur;
            break;
        case SEEK_END:
            seek_dir = std::ios_base::end;
            break;
        default:
            return -1;
    }

    io::seek(*static_cast<std::istream*>(input), delta, seek_dir);

    return 0;
}

int push_back_byte_impl(void* input, int c) {
    static_cast<std::istream*>(input)->putback(c);

    return c;
}

uint32_t get_length_impl(void* input) {
    auto ret = io::seek(*static_cast<std::istream*>(input), 0, std::ios_base::cur);
    auto cur = io::position_to_offset(ret);
    auto stream_length = io::position_to_offset(io::seek(*static_cast<std::istream*>(input), 0, std::ios_base::end));
    io::seek(*static_cast<std::istream*>(input), cur, std::ios_base::beg);

    return stream_length;
}

int can_seek_impl(void* input) {
    return true;
}

WavpackDecoder::WavpackDecoder(std::unique_ptr<std::istream> input)
    : m_input(std::move(input)), m_stream_reader({.read_bytes = read_bytes_impl,
                                                  .get_pos = get_pos_impl,
                                                  .set_pos_abs = set_pos_abs_impl,
                                                  .set_pos_rel = set_pos_rel_impl,
                                                  .push_back_byte = push_back_byte_impl,
                                                  .get_length = get_length_impl,
                                                  .can_seek = can_seek_impl}) {
    assert(m_input != nullptr);
    std::array<char, 255> error;
    error.fill(0);
    m_wavpack.reset(WavpackOpenFileInputEx(&m_stream_reader, m_input.get(), nullptr, error.data(), 0, 0));
    if(!m_wavpack) {
        BOOST_THROW_EXCEPTION(DecoderInitException() << ErrorTag::Plugin::Info(::wavpackInfo)
                                                     << ErrorTag::DecodeErrStr(error.data()));
    }

    as.bps = WavpackGetBitsPerSample(m_wavpack.get());
    as.sample_rate = WavpackGetSampleRate(m_wavpack.get());
    as.channels = WavpackGetNumChannels(m_wavpack.get());
}

WavpackDecoder::~WavpackDecoder() {
}

void WavpackDecoder::seek(chrono::milliseconds dur) {
    WavpackSeekSample(m_wavpack.get(), as.time_to_samples(dur));
}

chrono::milliseconds WavpackDecoder::tell() const {
    return as.samples_to_time<chrono::milliseconds>(WavpackGetSampleIndex(m_wavpack.get()));
}

chrono::milliseconds WavpackDecoder::duration() const {
    return as.samples_to_time<chrono::milliseconds>(WavpackGetNumSamples(m_wavpack.get()));
}

AudioSpecs WavpackDecoder::getAudioSpecs() const {
    return as;
}

size_t WavpackDecoder::decode(PCMBuffer& pcm_buf, std::error_code& ec) {
    pcm_buf.audio_specs = as;
    const auto bytes_requested = asio::buffer_size(pcm_buf);
    const auto samples_requested = as.bytes_to_samples(bytes_requested);
    int32_t buf[bytes_requested];
    auto samples_returned = WavpackUnpackSamples(m_wavpack.get(), buf, samples_requested);
    const auto bytes_returned = as.samples_to_bytes(samples_returned);

    if(bytes_returned != bytes_requested) {
        ec = asio::error::eof;
        if(WavpackGetNumErrors(m_wavpack.get()) > 0) {
            BOOST_THROW_EXCEPTION(DecoderException()
                                  << ErrorTag::Plugin::Info(::wavpackInfo)
                                  << ErrorTag::DecodeErrStr(WavpackGetErrorMessage(m_wavpack.get())));
        }
    }

    for(auto i = 0u; i < samples_returned; i++) {
        switch(as.bps) {
            case 8:
                asio::buffer_cast<pcm_sample<8>*>(pcm_buf)[i] = buf[i] + 0x80;
                break;
            case 16:
                asio::buffer_cast<pcm_sample<16>*>(pcm_buf)[i] = buf[i];
                break;
            case 24:
                asio::buffer_cast<pcm_sample<24>*>(pcm_buf)[i] = buf[i];
                break;
            case 32:
                asio::buffer_cast<pcm_sample<32>*>(pcm_buf)[i] = buf[i];
                break;
            default:
                break;
        }
    }
    return bytes_returned;
}

bool WavpackDecoder::valid() const {
    return static_cast<bool>(m_input);
}

void WavpackDecoder::reset() {
}
