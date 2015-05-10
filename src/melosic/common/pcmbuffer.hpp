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

#include <asio/buffer.hpp>

#include <melosic/common/audiospecs.hpp>

namespace Melosic {

struct PCMBuffer : asio::mutable_buffer {
    using asio::mutable_buffer::mutable_buffer;

    AudioSpecs audio_specs;
};

struct ConstPCMBuffer : asio::const_buffer {
    using asio::const_buffer::const_buffer;

    AudioSpecs audio_specs;
};

} // end namespace Melosic

namespace asio {

inline size_t buffer_size(const Melosic::PCMBuffer& b) {
    return buffer_size(mutable_buffer(b));
}
inline size_t buffer_size(const Melosic::ConstPCMBuffer& b) {
    return buffer_size(const_buffer(b));
}
}

#endif // MELOSIC_PCMBUFFER_HPP
