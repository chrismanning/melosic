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

#include <melosic/managers/common.hpp>
#include <FLAC++/decoder.h>
#include <string.h>
#include <boost-1_49/boost/shared_ptr.hpp>
#include <cstdlib>
#include <cstdio>
#include <iostream>

class FlacDecoderImpl : public FLAC::Decoder::File {
public:
    FlacDecoderImpl(IInputSource& dec_) : dec(dec_) {}
    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
    {
        ubyte * tmp = (ubyte *)malloc(frame->header.blocksize * sizeof(uint) * frame->header.channels / (frame->header.bits_per_sample/8));
        ubyte * buf = tmp;

        if(tmp == NULL) {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }

        for(unsigned i=0,u=0; i<frame->header.blocksize && u<(frame->header.blocksize*2); i++) {
            for(unsigned j=0; j<frame->header.channels; j++,u++) {
                switch(frame->header.bits_per_sample/8) {
                case 1:
                    *tmp = (ubyte)(buffer[j][i]);
                    tmp++;
                    break;
                case 2:
                    *(ushort*)tmp = (ushort)(buffer[j][i]);
                    tmp+=sizeof(ushort);
                    break;
                case 3:
                    *(uint*)tmp = (uint)(buffer[j][i] & 0xFFFFFF);
                    tmp+=3;
                    break;
                case 4:
                    *(uint*)tmp = (uint)(buffer[j][i]);
                    tmp+=sizeof(uint);
                    break;
                }
            }
        }

        dec.writeBuf(buf, frame->header.blocksize * frame->header.channels * (frame->header.bits_per_sample/8));
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);

    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) {
        //FIXME: use future error handling capabilities
        std::cerr << "Got error callback:" << FLAC__StreamDecoderErrorStatusString[status] << std::endl;
    }

private:
    IInputSource& dec;
};

class FlacDecoder : public IInputSource {
public:
    FlacDecoder() : as(0), fd(new FlacDecoderImpl(*this)), buf(new Buffer) {
    }

    ~FlacDecoder() {
        fprintf(stderr, "Flac decoder being destroyed\n");
        fd->finish();
        delete fd;
        if(as) {
            delete as;
        }
    }

    virtual void openFile(const char * filename) {
        fd->init(filename);
        fd->process_until_end_of_metadata();
    }

    void writeBuf(const void * ptr, size_t length) {
        if(buf->ptr()) {
            free(const_cast<void*>(buf->ptr()));
            buf->ptr(0);
        }
        buf->ptr(ptr);
        buf->length(length);
    }

    virtual AudioSpecs getAudioSpecs() {
        return *as;
    }

    virtual DecodeRange * getDecodeRange();

    AudioSpecs * as;
    FlacDecoderImpl * fd;
    IBuffer * buf;
};

void FlacDecoderImpl::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        //the get*() functions don't seem to work at this point
        FLAC__StreamMetadata_StreamInfo m = metadata->data.stream_info;
        auto tmp = new AudioSpecs(m.channels, m.bits_per_sample, m.sample_rate, m.total_samples);
        reinterpret_cast<FlacDecoder&>(dec).as = tmp;

        printf("sample rate    : %u Hz\n", tmp->sample_rate);
        printf("channels       : %u\n", tmp->channels);
        printf("bits per sample: %u\n", tmp->bps);
        printf("total samples  : %lu\n", tmp->total_samples);
    }
}

class FlacRange : public DecodeRange {
public:
    FlacRange(FlacDecoder * dec) : dec(dec) {
    }

    virtual IBuffer * front() {
        return dec->buf;
    }

    virtual void popFront() {
        if(!dec->fd->process_single()) {
            std::cerr << "Decode aborted" << std::endl;
        }
    }

    virtual bool empty() {
        auto state = dec->fd->get_state();
        return state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED;
    }

    virtual size_t length() {
        return dec->fd->get_total_samples();
    }

private:
    FlacDecoder * dec;
};

DecodeRange * FlacDecoder::getDecodeRange() {
    return new FlacRange(this);
}

class FlacFactory : public IInputFactory {
    virtual bool canOpen(const char * extension) {
        return strcmp(extension, ".flac") == 0;
    }
    virtual IInputSource * create() {
        return new FlacDecoder;
    }
};

extern "C" void registerPluginObjects(IKernel * k) {
    k->getInputManager()->addInputFactory(new FlacFactory);
}

extern "C" void destroyPluginObjects() {
    fprintf(stderr, "Destroying flac plugin objects\n");
}
