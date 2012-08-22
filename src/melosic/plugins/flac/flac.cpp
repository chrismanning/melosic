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
#include <boost/iostreams/stream_buffer.hpp>
namespace io = boost::iostreams;
#include <deque>
#include <algorithm>

#include <melosic/common/common.hpp>
#include <melosic/common/stream.hpp>
using namespace Melosic;

class FlacDecoderImpl : public FLAC::Decoder::Stream {
public:
    FlacDecoderImpl(IO::BiDirectionalSeekable& file, std::deque<char>& buf, AudioSpecs& as)
        : file(file), buf(buf), as(as)
    {
        io::seek(file, 0, std::ios_base::beg, std::ios_base::in);
        enforceEx<Exception>(init() == FLAC__STREAM_DECODER_INIT_STATUS_OK, "FLAC: Cannot initialise decoder");
        enforceEx<Exception>(process_until_end_of_metadata(), "FLAC: Processing of metadata failed");
    }

    virtual ~FlacDecoderImpl() {
        cerr << "Flac decoder impl being destroyed\n";
        finish();
    }

    bool end() {
        auto state = (::FLAC__StreamDecoderState)get_state();
        return state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED;
    }

    virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes) {
        try {
            *bytes = io::read(file, (char*)buffer, *bytes);

            if(*(std::streamsize*)bytes == -1) {
                *bytes = 0;
                return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
            }
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
        catch(std::ios_base::failure& e) {
            cerr << e.what() << endl;
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
    }

    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
                                                            const FLAC__int32 * const buffer[]) {
        for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*as.channels); i++) {
            for(unsigned j=0; j<frame->header.channels; j++,u++) {
                switch(frame->header.bits_per_sample/8) {
                    case 1:
                        buf.push_back((char)(buffer[j][i]));
                        break;
                    case 2:
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                        break;
                    case 3:
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                        buf.push_back((char)(buffer[j][i] >> 16));
                        break;
                    case 4:
                        buf.push_back((char)(buffer[j][i]));
                        buf.push_back((char)(buffer[j][i] >> 8));
                        buf.push_back((char)(buffer[j][i] >> 16));
                        buf.push_back((char)(buffer[j][i] >> 24));
                        break;
                }
            }
        }

        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) {
        cerr << "Got error callback: " << FLAC__StreamDecoderErrorStatusString[status] << endl;
        throw Exception(FLAC__StreamDecoderErrorStatusString[status]);
    }

    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata) {
        if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            //the get*() functions don't seem to work at this point
            FLAC__StreamMetadata_StreamInfo m = metadata->data.stream_info;

            as = AudioSpecs(m.channels, m.bits_per_sample, m.sample_rate, m.total_samples);

            cout << format("sample rate    : %u Hz") % as.sample_rate << endl;
            cout << format("channels       : %u") % (uint16_t)as.channels << endl;
            cout << format("bits per sample: %u") % (uint16_t)as.bps << endl;
            cout << format("total samples  : %u") % as.total_samples << endl;
        }
    }
private:
    IO::BiDirectionalSeekable& file;
    std::deque<char>& buf;
    AudioSpecs& as;
};

class FlacDecoder : public Input::IFileSource {
public:
    FlacDecoder(IO::BiDirectionalSeekable& file) : pimpl(new FlacDecoderImpl(file, buf, as)) {}

    virtual ~FlacDecoder() {
        cerr << "Flac decoder being destroyed\n";
    }

    virtual std::streamsize read(char * s, std::streamsize n) {
        while((std::streamsize)buf.size() < n && *this) {
            enforceEx<Exception>(pimpl->process_single() && !buf.empty(), "FLAC: Fatal processing error");
        }

        auto m = std::move(buf.begin(), buf.begin() + std::min(n, (std::streamsize)buf.size()), s);
        for(int i=0; i<m-s; i++) {
            buf.pop_front();
        }

        auto r = m - s;

        return r == 0 && !(*this) ? -1 : r;
    }

    virtual const AudioSpecs& getAudioSpecs() {
        return as;
    }

    virtual explicit operator bool() {
        return !pimpl->end();
    }

private:
    AudioSpecs as;
    std::deque<char> buf;
    std::shared_ptr<FlacDecoderImpl> pimpl;
};

extern "C" void registerPluginObjects(Kernel& k) {
    k.getInputManager().addFactory(factory<std::shared_ptr<FlacDecoder>>(), {".flac"});
}

extern "C" void destroyPluginObjects() {
    cerr << "Destroying flac objects\n";
}
