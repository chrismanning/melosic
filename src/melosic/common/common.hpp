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

#ifndef MELOSIC_COMMON_HPP
#define MELOSIC_COMMON_HPP

#include <iostream>
using std::cout; using std::cerr; using std::endl;
#include <boost/cstdint.hpp>
using boost::int64_t; using boost::uint64_t;

#include <melosic/managers/input/pluginterface.hpp>
#include <melosic/managers/output/pluginterface.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/common/plugin.hpp>
#include <melosic/common/logging.hpp>

#ifdef WIN32
#include <boost/thread.hpp>

namespace std {
  using boost::mutex;
  using boost::recursive_mutex;
  using boost::lock_guard;
  using boost::condition_variable;
  using boost::unique_lock;
  using boost::thread;
  namespace this_thread = boost::this_thread;
}
#endif

namespace Melosic {

struct AudioSpecs {
    AudioSpecs() : channels(0), bps(0), sample_rate(0), total_samples(0) {}
    AudioSpecs(uint8_t channels, uint8_t bps, uint32_t sample_rate, uint64_t total_samples)
        : channels(channels),
          bps(bps),
          sample_rate(sample_rate),
          target_sample_rate(sample_rate),
          total_samples(total_samples) {}

    bool operator==(const AudioSpecs& b) const {
        return channels == b.channels &&
               bps == b.bps &&
               sample_rate == b.sample_rate &&
               target_bps == b.target_bps &&
               target_sample_rate == b.target_sample_rate;
    }
    bool operator!=(const AudioSpecs& b) const {
        return !((*this) == b);
    }

    uint8_t channels;
    uint8_t bps;
    uint8_t target_bps = 0;
    uint32_t sample_rate;
    uint8_t target_sample_rate = 0;
    uint64_t total_samples;
};

namespace ErrorTag {
typedef boost::error_info<struct tagBPS, uint8_t> BPS;
typedef boost::error_info<struct tagChannels, uint8_t> Channels;
typedef boost::error_info<struct tagSampleRate, uint8_t> SampleRate;
}

} // end namespace Melosic

#endif // MELOSIC_COMMON_HPP
