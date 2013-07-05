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

#include <boost/functional/factory.hpp>
using boost::factory;
#include <boost/format.hpp>
using boost::format;
#include <boost/iostreams/read.hpp>
#include <boost/iostreams/positioning.hpp>
namespace io = boost::iostreams;
#include <deque>
#include <algorithm>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/common/stream.hpp>
#include <melosic/common/error.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/audiospecs.hpp>
using namespace Melosic;
using Logger::Severity;

static Logger::Logger logject(logging::keywords::channel = "FLAC");

static constexpr Plugin::Info flacInfo("FLAC",
                                   Plugin::Type::decode,
                                   {1,0,0});

#define FLAC_THROW_IF(Exc, cond, flacptr) if(!cond) {\
    BOOST_THROW_EXCEPTION(Exc() << ErrorTag::Plugin::Info(::flacInfo)\
                                << ErrorTag::DecodeErrStr(flacptr->get_state().as_cstring()));\
}

class FlacDecoderImpl : public FLAC::Decoder::Stream {
public:
    FlacDecoderImpl(IO::SeekableSource& input, std::deque<char>& buf, AudioSpecs& as)
        : input(input), buf(buf), as(as), lastSample(0)
    {
        FLAC_THROW_IF(DecoderInitException, init() == FLAC__STREAM_DECODER_INIT_STATUS_OK, this);
        FLAC_THROW_IF(MetadataException, process_until_end_of_metadata(), this);
        start = input.tellg();
        FLAC_THROW_IF(AudioDataInvalidException, process_single() && seek_absolute(0), this);
        reset();
        buf.clear();
    }

    virtual ~FlacDecoderImpl() {
        finish();
    }

    bool end() {
        auto state = (::FLAC__StreamDecoderState)get_state();
        return state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED;
    }

    virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes) {
        try {
            *bytes = io::read(input, (char*)buffer, *bytes);

            if(*(std::streamsize*)bytes == -1) {
                *bytes = 0;
                return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
            }
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

    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
                                                            const FLAC__int32 * const buffer[]) {
        lastSample = frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER
                     ? frame->header.number.frame_number : frame->header.number.sample_number;
        lastSample += frame->header.blocksize * as.channels;
        switch(frame->header.bits_per_sample) {
            case 8:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++) {
                    for(unsigned j=0; j<frame->header.channels; j++,u++) {
                        buf.push_back((char)(buffer[j][i]));
                    }
                }
                break;
            case 16:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++) {
                    for(unsigned j=0; j<frame->header.channels; j++,u++) {
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                    }
                }
                break;
            case 24:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++) {
                    for(unsigned j=0; j<frame->header.channels; j++,u++) {
                        if(as.target_bps == 32)
                            buf.push_back(0);
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                        buf.push_back((char)(buffer[j][i] >> 16));
                    }
                }
                break;
            case 32:
                for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++) {
                    for(unsigned j=0; j<frame->header.channels; j++,u++) {
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                        buf.push_back((char)(buffer[j][i] >> 16));
                        buf.push_back((char)(buffer[j][i] >> 24));
                    }
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

    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) {
        BOOST_THROW_EXCEPTION(DecoderException()
                              << ErrorTag::Plugin::Info(::flacInfo)
                              << ErrorTag::DecodeErrStr(FLAC__StreamDecoderErrorStatusString[status]));
    }

    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata) {
        if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            //the get*() functions don't seem to work at this point
            FLAC__StreamMetadata_StreamInfo m = metadata->data.stream_info;

            as = AudioSpecs(m.channels, m.bits_per_sample, m.sample_rate, m.total_samples);

            TRACE_LOG(logject) << format("sample rate    : %u Hz") % as.sample_rate;
            TRACE_LOG(logject) << format("channels       : %u") % (uint16_t)as.channels;
            TRACE_LOG(logject) << format("bits per sample: %u") % (uint16_t)as.bps;
            TRACE_LOG(logject) << format("total samples  : %u") % as.total_samples;
        }
    }

    virtual ::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset) {
        auto off = io::position_to_offset(input.seekg(absolute_byte_offset, std::ios_base::beg));
        if(off == (int64_t)absolute_byte_offset) {
//            std::clog << "In seek callback: " << input.seek(0, std::ios_base::cur, std::ios_base::in) << std::endl;
            return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
        }
        else {
            return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
        }
    }

    virtual ::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset) {
        *absolute_byte_offset = io::position_to_offset(input.tellg());
        return FLAC__STREAM_DECODER_TELL_STATUS_OK;
    }

    virtual ::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length) {
        auto cur = io::position_to_offset(input.tellg());
        *stream_length = io::position_to_offset(input.seekg(0, std::ios_base::end));
        input.seekg(cur, std::ios_base::beg);
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }

    void seek(chrono::milliseconds dur) {
        auto rate = get_sample_rate() / 1000.0;
        auto req = uint64_t(dur.count() * rate);
        if(!seek_absolute(req)) {
            WARN_LOG(logject) << "Seek to " << dur.count() << "ms failed";
            TRACE_LOG(logject) << "Position is " << tell().count() << "ms";
        }
//        else if(tell() != dur) {
//            std::clog << "Seek missed\nRequested: " << dur.count() << "; Got: " << tell().count() << std::endl;
//        }
    }

    chrono::milliseconds tell() {
        auto rate = get_sample_rate() / 1000.0;
        chrono::milliseconds time(int(lastSample / rate));
        return time;
    }

private:
    IO::SeekableSource& input;
    std::deque<char>& buf;
    AudioSpecs& as;
    std::streampos start;
    uint64_t lastSample;
};

class FlacDecoder : public Decoder::Playable {
public:
    FlacDecoder(IO::SeekableSource& input) :
        pimpl(new FlacDecoderImpl(input, buf, as))
    {}

    virtual ~FlacDecoder() {}

    virtual std::streamsize read(char * s, std::streamsize n) {
        while((std::streamsize)buf.size() < n && *this) {
            FLAC_THROW_IF(AudioDataInvalidException, pimpl->process_single() && !buf.empty(), pimpl.get());
        }

        auto min = std::min(n, (std::streamsize)buf.size());
        auto m = std::move(buf.begin(), buf.begin() + min, s);
        auto d = std::distance(s, m);
        buf.erase(buf.begin(), buf.begin() + d);

        return d == 0 && !(*this) ? -1 : d;
    }

    virtual void seek(chrono::milliseconds dur) {
        pimpl->seek(dur);
    }

    chrono::milliseconds tell() {
        return pimpl->tell();
    }

    virtual void reset() {
        pimpl->reset();
        seek(chrono::milliseconds(0));
        buf.clear();
    }

    virtual AudioSpecs& getAudioSpecs() {
        return as;
    }

    virtual explicit operator bool() {
        return !pimpl->end();
    }

private:
    AudioSpecs as;
    std::deque<char> buf;
    std::unique_ptr<FlacDecoderImpl> pimpl;
};

extern "C" MELOSIC_PLUGIN_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::flacInfo;
    funs << registerDecoder;
}

extern "C" MELOSIC_PLUGIN_EXPORT void registerDecoder(Decoder::Manager* decman) {
    decman->addAudioFormat(factory<std::unique_ptr<FlacDecoder>>(), ".flac");
}

extern "C" MELOSIC_PLUGIN_EXPORT void destroyPlugin() {
}
