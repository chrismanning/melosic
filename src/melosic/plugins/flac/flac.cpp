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
#include <boost/circular_buffer.hpp>
using boost::circular_buffer;
#include <vector>

#include <melosic/common/common.hpp>
using namespace Melosic;

class FlacDecoder : public Input::IFileSource, private FLAC::Decoder::File {
public:
    FlacDecoder() : circ_buf(65536) {}

    virtual ~FlacDecoder() {
        cerr << "Flac decoder being destroyed\n" << endl;
        finish();
    }

    virtual std::streamsize read(uint8_t * s, std::streamsize n) {
        while((std::streamsize)circ_buf.size() < n && !end()) {
            process_single();
        }
        std::streamsize i;
        for(i = 0; i<n && !circ_buf.empty(); i++) {
            s[i] = circ_buf.front();
            circ_buf.pop_front();
        }
        return i;
    }

    virtual void openFile(const std::string& filename) {
        init(filename);
        process_until_end_of_metadata();
    }

    virtual const AudioSpecs& getAudioSpecs() {
        return as;
    }

private:
    bool end() {
        auto state = (::FLAC__StreamDecoderState)get_state();
        return state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED;
    }

    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
                                                            const FLAC__int32 * const buffer[]) {
        auto bytes = frame->header.blocksize
                     * sizeof(int32_t)
                     * frame->header.channels
                     / (frame->header.bits_per_sample/8);

        std::vector<uint8_t> buf_(bytes);
        auto tmp = &buf_[0];

        if(tmp == NULL) {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }

        for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*2); i++) {
            for(unsigned j=0; j<frame->header.channels; j++,u++) {
                switch(frame->header.bits_per_sample/8) {
                    case 1:
                        circ_buf.push_back((uint8_t)(buffer[j][i]));
                        break;
                    case 2:
                        circ_buf.push_back((uint8_t)(buffer[j][i]));
                        circ_buf.push_back((uint8_t)(buffer[j][i] >> 8));
                        break;
                    case 3:
                        circ_buf.push_back((uint8_t)(buffer[j][i]));
                        circ_buf.push_back((uint8_t)(buffer[j][i] >> 8));
                        circ_buf.push_back((uint8_t)(buffer[j][i] >> 16));
                        break;
                    case 4:
                        circ_buf.push_back((uint8_t)(buffer[j][i]));
                        circ_buf.push_back((uint8_t)(buffer[j][i] >> 8));
                        circ_buf.push_back((uint8_t)(buffer[j][i] >> 16));
                        circ_buf.push_back((uint8_t)(buffer[j][i] >> 24));
                        break;
                }
            }
        }

        buf = std::move(buf_);

        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) {
        //FIXME: use future error handling capabilities
        cerr << "Got error callback:" << FLAC__StreamDecoderErrorStatusString[status] << endl;
    }

    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata) {
        if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            //the get*() functions don't seem to work at this point
            FLAC__StreamMetadata_StreamInfo m = metadata->data.stream_info;

            as = AudioSpecs(m.channels, m.bits_per_sample, m.sample_rate, m.total_samples);

            cout << format("sample rate    : %u Hz") % as.sample_rate << endl;
            cout << format("channels       : %u") % as.channels << endl;
            cout << format("bits per sample: %u") % as.bps << endl;
            cout << format("total samples  : %lu") % as.total_samples << endl;
        }
    }

    AudioSpecs as;
    std::vector<uint8_t> buf;
    circular_buffer<uint8_t> circ_buf;
};

extern "C" void registerPluginObjects(IKernel& k) {
    k.getInputManager().addFactory([]() -> FlacDecoder&& {return std::move(FlacDecoder());}, {".flac"});
}

extern "C" void destroyPluginObjects() {
    std::cerr << "Destroying flac objects" << std::endl;
}
