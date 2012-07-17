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

#include <melosic/managers/common.h>
#include <FLAC++/decoder.h>
#include <string.h>
#include <iostream>

class FlacDecoderImpl : public FLAC::Decoder::File {
public:
    FlacDecoderImpl(IInput& dec_) : dec(dec_) {}
    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
    {
        for(unsigned i=0; i<frame->header.blocksize; i++) {
            for(unsigned j=0; j<frame->header.channels; j++) {
                //FIXME: don't use output range directly once output interface is devised
                if(dec.getOutputRange()->put((short)buffer[j][i], frame->header.bits_per_sample))
                    continue;
                else
                    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
            }
        }
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);

    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) {
        std::cerr << "Got error callback:" << FLAC__StreamDecoderErrorStatusString[status] << std::endl;
    }

private:
    IInput& dec;
};

class FlacDecoder : public IInput {
public:
    FlacDecoder() : fd(new FlacDecoderImpl(*this)) {
    }

    ~FlacDecoder() {
        fd->finish();
        delete fd;
        delete as;
    }

    virtual bool canOpen(const char * extension) {
        return strcmp(extension, ".flac") == 0;
    }

    virtual void openFile(const char * filename) {
        fd->init(filename);
        fd->process_until_end_of_metadata();
    }

    virtual void initOutput(IOutputRange * output_) {
        output = output_;
    }

    virtual IOutputRange * getOutputRange() {
        return output;
    }

    virtual AudioSpecs getAudioSpecs() {
        return *as;
    }

    virtual DecodeRange * opSlice();

    AudioSpecs * as;
    FlacDecoderImpl * fd;
    unsigned * buf;
    IOutputRange * output;
};

void FlacDecoderImpl::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
    /* print some stats */
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        auto tmp = new AudioSpecs(get_channels(),get_bits_per_sample(),get_total_samples(),get_sample_rate());
        reinterpret_cast<FlacDecoder&>(dec).as = tmp;

        fprintf(stderr, "sample rate    : %u Hz\n", tmp->sample_rate);
        fprintf(stderr, "channels       : %u\n", tmp->channels);
        fprintf(stderr, "bits per sample: %u\n", tmp->bps);
        fprintf(stderr, "total samples  : %lu\n", tmp->total_samples);
    }
}

class FlacRange : public DecodeRange {
public:
    FlacRange(FlacDecoder * dec) : dec(dec) {
    }

    ~FlacRange() {
        delete dec;
    }

    virtual unsigned * front() {
        return dec->buf;
    }

    virtual void popFront() {
        printf("in popFront (C++)\n");
        dec->fd->process_single();
    }

    virtual bool empty() {
        auto state = dec->fd->get_state();
        return state == FLAC__STREAM_DECODER_END_OF_STREAM;
    }

private:
    FlacDecoder * dec;
};

DecodeRange * FlacDecoder::opSlice() {
    return new FlacRange(this);
}

extern "C" void registerPlugin(IKernel * k) {
    k->getInputManager()->addDecoder(new FlacDecoder);
}
