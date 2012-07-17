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
    FlacDecoderImpl(IInputDecoder& outer) : outer(outer) {}
    ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const int * buffer[])
    {
        if(outer.writeCallback(frame->header.blocksize, (unsigned short)frame->header.channels, buffer))
            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        else
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    IInputDecoder& outer;
};

class FlacDecoder : public IInputDecoder {
public:
    bool writeCallback(unsigned chunkSize, unsigned short channels, const int * buffer[]) {
        return true;
    }
    bool canOpen(const char * extension) {
        return strcmp(extension, ".flac") == 0;
    }

    void openFile(const char * filename) {
        //init(filename);
    }
};

extern "C" void registerPlugin(IKernel * k) {
    k->getInputManager()->addDecoder(new FlacDecoder);
}
