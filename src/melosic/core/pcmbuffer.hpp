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

#ifndef MELOSIC_PCMBUFFER_HPP
#define MELOSIC_PCMBUFFER_HPP

#include <melosic/common/common.hpp>
#include <melosic/common/properties.hpp>

namespace Melosic {

class PCMBuffer {
public:
    PCMBuffer();

    void reserve(size_t samples);
    void pushSample(int sample, uint8_t bits);

    readWriteProperty(uint8_t, bitDepth, BitDepth)
    readWriteProperty(uint32_t, sampleRate, SampleRate)
    readWriteProperty(AudioSpecs, audioSpecs, AudioSpecs)
};

} //end namespace Melosic

#endif // MELOSIC_PCMBUFFER_HPP
