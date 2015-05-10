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

#ifndef MELOSIC_AUDIOSPECS_HPP
#define MELOSIC_AUDIOSPECS_HPP

#include <melosic/common/common.hpp>

namespace Melosic {

struct AudioSpecs final {
    constexpr AudioSpecs() noexcept = default;
    constexpr AudioSpecs(uint8_t channels, uint8_t bps, uint32_t sample_rate) noexcept : channels(channels),
                                                                                         bps(bps),
                                                                                         sample_rate(sample_rate) {
    }

    constexpr bool operator==(const AudioSpecs& b) const noexcept {
        return channels == b.channels && bps == b.bps && sample_rate == b.sample_rate;
    }
    constexpr bool operator!=(const AudioSpecs& b) const noexcept {
        return !((*this) == b);
    }

    constexpr size_t samples_to_bytes(size_t samples) const noexcept {
        return samples * (bps / 8) * channels;
    }

    constexpr size_t bytes_to_samples(size_t bytes) const noexcept {
        return bytes / (bps / 8) / channels;
    }

    template <typename Duration> constexpr size_t time_to_samples(Duration time) const noexcept {
        return sample_rate * chrono::duration_cast<chrono::duration<double>>(time).count();
    }

    template <typename Duration> constexpr Duration samples_to_time(size_t samples) const noexcept {
        return chrono::duration_cast<Duration>(chrono::duration<double>(samples / static_cast<double>(sample_rate)));
    }

    template <typename Duration> constexpr size_t time_to_bytes(Duration time) const noexcept {
        return samples_to_bytes(time_to_samples(time));
    }

    template <typename Duration> constexpr Duration bytes_to_time(size_t bytes) const noexcept {
        return samples_to_time<Duration>(bytes_to_samples(bytes));
    }

    uint8_t channels = 0;
    uint8_t bps = 0;
    uint32_t sample_rate = 0;
};

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, const AudioSpecs& as) {
    os << "channels: " << static_cast<unsigned>(as.channels);
    os << "; bps: " << static_cast<unsigned>(as.bps);
    os << "; sample_rate: " << as.sample_rate;
    return os;
}

} // end namespace Melosic

#endif // MELOSIC_AUDIOSPECS_HPP
