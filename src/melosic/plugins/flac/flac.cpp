/**************************************************************************
**  Copyright (C) 2012 Christian Manning
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

#include <FLAC++/decoder.h>

#include <deque>
#include <algorithm>
#include <cmath>
#include <fstream>

#include <boost/config.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/seek.hpp>
namespace io = boost::iostreams;
#include <asio/error.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/common/error.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/common/pcmbuffer.hpp>
using namespace Melosic;
using Logger::Severity;
namespace ASIO = asio;

static Logger::Logger logject{logging::keywords::channel = "FLAC"};

static constexpr Plugin::Info flacInfo{"FLAC",
                                       Plugin::Type::decode,
                                       {1,0,0}};

#define FLAC_THROW_IF(Exc, cond, flacptr) if(!(cond)) {\
    BOOST_THROW_EXCEPTION(Exc() << ErrorTag::Plugin::Info(::flacInfo)\
                                << ErrorTag::DecodeErrStr(flacptr->get_state().as_cstring()));\
}

struct FlacDecoderImpl : FLAC::Decoder::Stream {
    FlacDecoderImpl(std::unique_ptr<std::istream> input, std::deque<char>& buf, AudioSpecs& as)
        : m_input(std::move(input)), buf(buf), as(as), lastSample(0)
    {
        assert(m_input != nullptr);
        m_input->exceptions(std::ifstream::badbit | std::ifstream::failbit);
        FLAC_THROW_IF(DecoderInitException, init() == FLAC__STREAM_DECODER_INIT_STATUS_OK, this);
        FLAC_THROW_IF(MetadataException, process_until_end_of_metadata(), this);
        start = io::seek(*m_input, 0, std::ios_base::cur);
        FLAC_THROW_IF(AudioDataInvalidException, process_single() && seek_absolute(0), this);
        reset();
        buf.clear();
    }

    virtual ~FlacDecoderImpl() {
        finish();
    }

    bool end() const {
        auto state = (::FLAC__StreamDecoderState)get_state();
        return state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED;
    }

    ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes) override {
        try {
            auto n = io::read(*m_input, (char*)buffer, *bytes);

            if(n < 0) {
                *bytes = 0;
                return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
            }
            *bytes = static_cast<size_t>(+n);
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
        catch(ReadException& e) {
            ERROR_LOG(logject) << "Read error: decoder aborting";
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
        catch(std::ios_base::failure& e) {
            ERROR_LOG(logject) << e.what();
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
    }

    ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
                                                    const FLAC__int32 * const buffer[]) override {
        lastSample = frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER
                     ? frame->header.number.frame_number : frame->header.number.sample_number;
        lastSample += frame->header.blocksize * as.channels;
        switch(frame->header.bits_per_sample) {
            case 8:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++)
                    for(unsigned j=0; j<frame->header.channels; j++,u++)
                        buf.push_back((char)(buffer[j][i]));
                break;
            case 16:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++)
                    for(unsigned j=0; j<frame->header.channels; j++,u++) {
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                    }
                break;
            case 24:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++)
                    for(unsigned j=0; j<frame->header.channels; j++,u++) {
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                        buf.push_back((char)(buffer[j][i] >> 16));
                    }
                break;
            case 32:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++)
                    for(unsigned j=0; j<frame->header.channels; j++,u++) {
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                        buf.push_back((char)(buffer[j][i] >> 16));
                        buf.push_back((char)(buffer[j][i] >> 24));
                    }
                break;
            default:
                BOOST_THROW_EXCEPTION(AudioDataUnsupported()
                                      << ErrorTag::Plugin::Info(::flacInfo)
                                      << ErrorTag::BPS(frame->header.bits_per_sample));
                break;
        }

        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    void error_callback(::FLAC__StreamDecoderErrorStatus status) override {
        BOOST_THROW_EXCEPTION(DecoderException()
                              << ErrorTag::Plugin::Info(::flacInfo)
                              << ErrorTag::DecodeErrStr(FLAC__StreamDecoderErrorStatusString[status]));
    }

    void metadata_callback(const ::FLAC__StreamMetadata *metadata) override {
        if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            //the get*() functions don't seem to work at this point
            m_metadata_cache = metadata->data.stream_info;

            as = {static_cast<uint8_t>(m_metadata_cache.channels),
                  static_cast<uint8_t>(m_metadata_cache.bits_per_sample),
                  m_metadata_cache.sample_rate};

            TRACE_LOG(logject) << as;
        }
    }

    ::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset) override {
        auto off = io::position_to_offset(io::seek(*m_input, absolute_byte_offset, std::ios_base::beg));
        if(off == (int64_t)absolute_byte_offset)
            return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
        else
            return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }

    ::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset) override {
        *absolute_byte_offset = io::position_to_offset(io::seek(*m_input, 0, std::ios_base::cur));
        return FLAC__STREAM_DECODER_TELL_STATUS_OK;
    }

    ::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64* stream_length) override {
        auto cur = io::position_to_offset(io::seek(*m_input, 0, std::ios_base::cur));
        *stream_length = io::position_to_offset(io::seek(*m_input, 0, std::ios_base::end));
        io::seek(*m_input, cur, std::ios_base::beg);
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }

    bool eof_callback() override {
        return m_input->eof();
    }

    void seek(chrono::milliseconds dur) {
        if(!seek_absolute(as.time_to_samples(dur))) {
            WARN_LOG(logject) << "Seek to " << dur.count() << "ms failed";
            TRACE_LOG(logject) << "Position is " << tell().count() << "ms";
        }
    }

    chrono::milliseconds tell() const {
        return as.samples_to_time<chrono::milliseconds>(lastSample);
    }

    std::unique_ptr<std::istream> m_input;
    std::deque<char>& buf;
    AudioSpecs& as;
    std::streampos start;
    FLAC__StreamMetadata_StreamInfo m_metadata_cache;
    uint64_t lastSample;
};

class FlacDecoder : public Decoder::PCMSource {
public:
    FlacDecoder(std::unique_ptr<std::istream> input) :
        as(),
        buf(),
        impl(std::move(input), buf, as)
    {}

    virtual ~FlacDecoder() {}

    size_t decode(PCMBuffer& pcm_buf, std::error_code& ec) override {
        pcm_buf.audio_specs = as;
        while(buf.size() < ASIO::buffer_size(pcm_buf) && !impl.end()) {
            auto r = impl.process_single();
            if(!r || FLAC__STREAM_DECODER_END_OF_STREAM == static_cast<FLAC__StreamDecoderState>(impl.get_state())) {
                if(buf.empty()) {
                    ec = ASIO::error::make_error_code(ASIO::error::eof);
                    return 0;
                }
                else
                    break;
            }

            FLAC_THROW_IF(AudioDataInvalidException, r, (&impl));
        }

        auto min = std::min(ASIO::buffer_size(pcm_buf), buf.size());
        auto m = std::move(buf.begin(), std::next(buf.begin(), min), ASIO::buffer_cast<char*>(pcm_buf));
        auto d = std::distance(ASIO::buffer_cast<char*>(pcm_buf), m);
        buf.erase(buf.begin(), std::next(buf.begin(), d));

        return d == 0 && !valid() ? (-1) : d;
    }

    void seek(chrono::milliseconds dur) override {
        impl.seek(dur);
    }

    chrono::milliseconds tell() const override {
        return impl.tell();
    }

    chrono::milliseconds duration() const override {
        return as.samples_to_time<chrono::milliseconds>(impl.m_metadata_cache.total_samples);
    }

    void reset() override {
        impl.reset();
        seek(0ms);
        buf.clear();
    }

    AudioSpecs getAudioSpecs() const override {
        return as;
    }

    bool valid() const override {
        return !(impl.end() && buf.empty());
    }

private:
    AudioSpecs as;
    std::deque<char> buf;
    FlacDecoderImpl impl;
};

extern "C" BOOST_SYMBOL_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::flacInfo;
    funs << registerDecoder;
}

extern "C" BOOST_SYMBOL_EXPORT void registerDecoder(Decoder::Manager* decman) {
    decman->addAudioFormat([](auto input) { return std::make_unique<FlacDecoder>(std::move(input)); },
    boost::string_ref("audio/flac"), boost::string_ref("audio/x-flac"));
}

extern "C" BOOST_SYMBOL_EXPORT void destroyPlugin() {
}
