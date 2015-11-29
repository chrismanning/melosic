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

#ifndef MELOSIC_PCM_SAMPLE
#define MELOSIC_PCM_SAMPLE

#include <array>

#include <boost/integer.hpp>

namespace Melosic {

template <int bits> struct pcm_sample {
    static_assert(bits > 0 && bits <= 32);
    using int_t = typename boost::int_t<bits>::least;

    constexpr pcm_sample(int_t sample) noexcept {
        *this = sample;
    }

    constexpr pcm_sample& operator=(int_t sample) noexcept {
        for(auto i = 0u; i < arr.size(); ++i) {
            arr[i] = ((sample & mask(i)) >> (i * 8));
        }
        return *this;
    }

    constexpr pcm_sample& operator+=(int_t sample) noexcept {
        int_t base = *this;
        base += sample;
        return *this = base;
    }

    constexpr operator int_t() const noexcept {
        int_t base = 0;
        for(auto i = 0u; i < arr.size(); ++i) {
            base |= ((arr[i] << (i * 8)) & mask(i));
        }
        return base;
    }

    static constexpr int_t max = (2L << (bits - 2)) - 1;

  private:
    friend std::ostream& operator<<(std::ostream& os, pcm_sample<bits> data) {
        for(auto&& byte : data.arr) {
            os << byte;
        }
        return os;
    }

    static constexpr uint32_t mask(unsigned byte) noexcept {
        return 0xff << (byte * 8);
    }

    std::array<char, (bits / 8) + (bits % 8 != 0)> arr{{0}};
};

} // namespace Melosic

#endif // MELOSIC_PCM_SAMPLE
