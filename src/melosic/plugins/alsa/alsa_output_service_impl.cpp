/**************************************************************************
**  Copyright (C) 2016 Christian Manning
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

#include <poll.h>

#include <chrono>
using namespace std::literals;

#include "alsa_output_service_impl.hpp"

namespace alsa {

struct alsa_category_ : std::error_category {
    const char* name() const noexcept override {
        return "alsa";
    }

    std::string message(int errnum) const override {
        return snd_strerror(errnum);
    }
} alsa_category;

static constexpr std::array<snd_pcm_format_t, 5> formats{
    {SND_PCM_FORMAT_S8, SND_PCM_FORMAT_S16_LE, SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S24_LE, SND_PCM_FORMAT_S32_LE}};
static constexpr std::array<uint8_t, 5> bpss{{8, 16, 24, 24, 32}};

Melosic::Config::Conf conf{"ALSA"};
snd_pcm_uframes_t frames = 1024;
bool resample = false;
auto buf_time_msecs = 1000ms;

output_service_impl::output_service_impl(asio::io_service& service) : base_t(service), m_asio_fd(service) {
}

void output_service_impl::assign(Melosic::Output::device_descriptor dev_name, std::error_code& ec) {
    if(m_pdh != nullptr) {
        ec = {asio::error::already_open, asio::error::get_misc_category()};
        return;
    }
    m_name = dev_name.getName();
    m_desc = dev_name.getDesc();
}

void output_service_impl::destroy() {
    std::error_code ec;
    if(m_pdh != nullptr)
        stop(ec);
    if(m_params != nullptr)
        snd_pcm_hw_params_free(m_params);
}

void output_service_impl::cancel(std::error_code& ec) {
    if(m_asio_fd.is_open())
        m_asio_fd.cancel(ec);
}

Melosic::AudioSpecs output_service_impl::prepare(Melosic::AudioSpecs as, std::error_code& ec) {
    m_current_specs = as;
    m_state = Melosic::Output::DeviceState::Error;
    if((m_pdh == nullptr) &&
       (ec = {snd_pcm_open(&m_pdh, m_name.c_str(), SND_PCM_STREAM_PLAYBACK, m_non_blocking), alsa_category}))
        return m_current_specs;

    if((m_params == nullptr) && (ec = {snd_pcm_hw_params_malloc(&m_params), alsa_category}))
        return m_current_specs;

    snd_pcm_hw_params_any(m_pdh, m_params);

    snd_pcm_hw_params_set_access(m_pdh, m_params, SND_PCM_ACCESS_RW_INTERLEAVED);

    if((ec = {snd_pcm_hw_params_set_channels(m_pdh, m_params, m_current_specs.channels), alsa_category}))
        return m_current_specs;
    int dir = 0;
    if((ec = {snd_pcm_hw_params_set_rate_resample(m_pdh, m_params, resample), alsa_category}))
        return m_current_specs;
    if((ec = {snd_pcm_hw_params_set_rate_near(m_pdh, m_params, &m_current_specs.sample_rate, &dir), alsa_category}))
        return m_current_specs;
    if(as.sample_rate != m_current_specs.sample_rate)
        WARN_LOG(logject) << "Sample rate: " << as.sample_rate << " not supported. Using "
                          << m_current_specs.sample_rate;

    snd_pcm_format_t fmt(SND_PCM_FORMAT_UNKNOWN);

    switch(m_current_specs.bps) {
        case 8:
            fmt = SND_PCM_FORMAT_S8;
            break;
        case 16:
            fmt = SND_PCM_FORMAT_S16_LE;
            break;
        case 24:
            fmt = SND_PCM_FORMAT_S24_3LE;
            break;
        case 32:
            fmt = SND_PCM_FORMAT_S32_LE;
            break;
        default:
            BOOST_THROW_EXCEPTION(std::runtime_error("unknown bps"));
    }

    if(snd_pcm_hw_params_test_format(m_pdh, m_params, fmt) < 0) {
        WARN_LOG(logject) << "unsupported format";

        auto p = std::find(formats.begin(), formats.end(), fmt);
        auto bps = &bpss[std::distance(formats.begin(), p)];
        for(; p < formats.end(); p++, bps++) {
            WARN_LOG(logject) << "trying higher bps of " << static_cast<unsigned>(*bps);
            if(!snd_pcm_hw_params_test_format(m_pdh, m_params, *p)) {
                fmt = *p;
                m_current_specs.bps = *bps;
                WARN_LOG(logject) << "settled on bps of " << static_cast<unsigned>(m_current_specs.bps);
                break;
            }
        }

        if(p == formats.end()) {
            auto rp = std::find(formats.rbegin(), formats.rend(), fmt);
            bps = &bpss[std::distance(formats.rbegin(), rp)];
            for(; rp < formats.rend(); rp++, bps--) {
                WARN_LOG(logject) << "trying lower bps of " << static_cast<unsigned>(*bps);
                if(!snd_pcm_hw_params_test_format(m_pdh, m_params, *rp)) {
                    fmt = *rp;
                    m_current_specs.bps = *bps;
                    WARN_LOG(logject) << "settled on bps of " << static_cast<unsigned>(m_current_specs.bps);
                    break;
                }
            }
            if(rp == formats.rend()) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return m_current_specs;
            }
        }
    }

    if((ec = {snd_pcm_hw_params_set_format(m_pdh, m_params, fmt), alsa_category}))
        return m_current_specs;

    snd_pcm_uframes_t min_frames;
    snd_pcm_hw_params_get_period_size_min(m_params, &min_frames, &dir);
    if(min_frames == 0)
        min_frames = 1;
    TRACE_LOG(logject) << "min frames: " << min_frames;

    snd_pcm_uframes_t min_buf;
    snd_pcm_hw_params_get_buffer_size_min(m_params, &min_buf);
    if(min_buf == 0)
        min_buf = 1;
    TRACE_LOG(logject) << "min buf: " << min_buf;

    snd_pcm_uframes_t buf = m_current_specs.time_to_bytes(buf_time_msecs);
    TRACE_LOG(logject) << "buf: " << buf;
    if((ec = {snd_pcm_hw_params_set_buffer_size_near(m_pdh, m_params, &buf), alsa_category}))
        return m_current_specs;

    snd_pcm_hw_params_get_buffer_size(m_params, &buf);
    TRACE_LOG(logject) << "set buf: " << buf;

    snd_pcm_uframes_t frames = buf / (min_buf / min_frames);
    TRACE_LOG(logject) << "frames = buf/(min_buf/min_frames): " << frames;
    if((ec = {snd_pcm_hw_params_set_period_size_near(m_pdh, m_params, &frames, &dir), alsa_category}))
        return m_current_specs;
    TRACE_LOG(logject) << "set frames per period: " << frames;

    if((ec = {snd_pcm_hw_params(m_pdh, m_params), alsa_category}))
        return m_current_specs;

    auto pcount = snd_pcm_poll_descriptors_count(m_pdh);
    if((pcount < 0) && (ec = {pcount, alsa_category}))
        return m_current_specs;

    m_pfds.clear();
    m_pfds.resize(pcount);

    auto n = snd_pcm_poll_descriptors(m_pdh, m_pfds.data(), m_pfds.size());
    TRACE_LOG(logject) << "no. poll descriptors: " << m_pfds.size();
    TRACE_LOG(logject) << "no. poll descriptors filled: " << n;
    assert(!m_pfds.empty());
    if((n < 0) && (ec = {n, alsa_category}))
        return m_current_specs;

    m_mangled_revents = static_cast<bool>(m_pfds.data()->events & POLLIN);

    if(m_asio_fd.is_open()) {
        m_asio_fd.cancel();
        m_asio_fd.close();
    }
    if(m_asio_fd.assign(m_pfds.data()->fd, ec))
        return m_current_specs;

    {
        auto events = m_pfds.data()->events;
        if(events & POLLERR)
            TRACE_LOG(logject) << "events: POLLERR";
        if(events & POLLOUT)
            TRACE_LOG(logject) << "events: POLLOUT";
        if(events & POLLIN)
            TRACE_LOG(logject) << "events: POLLIN";
        if(events & POLLPRI)
            TRACE_LOG(logject) << "events: POLLPRI";
        if(events & POLLRDHUP)
            TRACE_LOG(logject) << "events: POLLRDHUP";
        if(events & POLLHUP)
            TRACE_LOG(logject) << "events: POLLHUP";
        if(events & POLLNVAL)
            TRACE_LOG(logject) << "events: POLLNVAL";
        if(events & POLLRDNORM)
            TRACE_LOG(logject) << "events: POLLRDNORM";
        if(events & POLLRDBAND)
            TRACE_LOG(logject) << "events: POLLRDBAND";
        if(events & POLLWRNORM)
            TRACE_LOG(logject) << "events: POLLWRNORM";
        if(events & POLLWRBAND)
            TRACE_LOG(logject) << "events: POLLWRBAND";
    }
    m_pfds.clear();

    m_state = Melosic::Output::DeviceState::Ready;
    assert(!ec);
    TRACE_LOG(logject) << "internal state after prepare: " << snd_pcm_state_name(snd_pcm_state(m_pdh));

    return m_current_specs;
}

void output_service_impl::play(std::error_code& ec) {
    switch(m_state) {
        case Melosic::Output::DeviceState::Stopped:
        case Melosic::Output::DeviceState::Error:
            prepare(m_current_specs, ec);
            if(ec)
                return;
            break;
        case Melosic::Output::DeviceState::Ready:
            if((ec = {snd_pcm_prepare(m_pdh), alsa_category}))
                return;
            m_state = Melosic::Output::DeviceState::Playing;
            break;
        case Melosic::Output::DeviceState::Paused:
            unpause(ec);
            if(ec)
                return;
            break;
        default:
            break;
    }
    assert(!ec);
}

void output_service_impl::pause(std::error_code& ec) {
    if(m_state == Melosic::Output::DeviceState::Playing) {
        if(snd_pcm_hw_params_can_pause(m_params)) {
            if((ec = {snd_pcm_pause(m_pdh, true), alsa_category}))
                return;
        } else {
            if((ec = {snd_pcm_drop(m_pdh), alsa_category}))
                return;
        }
        cancel(ec);
        m_state = Melosic::Output::DeviceState::Paused;
    } else if(m_state == Melosic::Output::DeviceState::Paused) {
        unpause(ec);
        if(ec)
            return;
    }
    assert(!ec);
}

void output_service_impl::unpause(std::error_code& ec) {
    if(snd_pcm_hw_params_can_pause(m_params)) {
        if((ec = {snd_pcm_pause(m_pdh, false), alsa_category}))
            return;
    } else {
        snd_pcm_prepare(m_pdh);
        snd_pcm_start(m_pdh);
    }
    m_state = Melosic::Output::DeviceState::Playing;
    assert(!ec);
}

void output_service_impl::stop(std::error_code& ec) {
    if(m_state != Melosic::Output::DeviceState::Stopped && m_state != Melosic::Output::DeviceState::Error && m_pdh) {
        cancel(ec);
        if((ec = {snd_pcm_drop(m_pdh), alsa_category}))
            return;
        if((ec = {snd_pcm_close(m_pdh), alsa_category}))
            return;
    }
    m_pfds.clear();
    m_asio_fd.release();
    m_state = Melosic::Output::DeviceState::Stopped;
    m_pdh = nullptr;
    assert(!ec);
}

size_t output_service_impl::write_some(const asio::const_buffer& buf, std::error_code& ec) {
    if(m_pdh == nullptr)
        ec = asio::error::make_error_code(asio::error::bad_descriptor);

    if(ec)
        return 0;

    const auto size = asio::buffer_size(buf);
    auto ptr = asio::buffer_cast<const char*>(buf);

    if(size == 0 || ptr == nullptr)
        return 0;

    auto frames = snd_pcm_bytes_to_frames(m_pdh, size);
    auto r = snd_pcm_writei(m_pdh, ptr, frames);

    if(r < 0) {
        if(m_pdh != nullptr)
            r = snd_pcm_recover(m_pdh, r, false);
        ec = {static_cast<int>(r), alsa_category};
        return 0;
    }

    assert(m_pdh != nullptr);
    r = snd_pcm_frames_to_bytes(m_pdh, r);

    return r;
}

void output_service_impl::async_prepare(const Melosic::AudioSpecs,
                                        Melosic::AudioIO::AudioOutputServiceBase::PrepareHandler) {
}

void output_service_impl::async_write_some(const asio::const_buffer& buf,
                                           Melosic::AudioIO::AudioOutputServiceBase::WriteHandler handler) {
    auto f = [=](std::error_code ec) {
        size_t n = 0;
        if(!ec)
            n = write_some(buf, ec);
        handler(ec, n);
    };

    m_asio_fd.async_wait(m_mangled_revents ? asio::posix::stream_descriptor::wait_read
                                           : asio::posix::stream_descriptor::wait_write,
                         std::move(f));
}

Melosic::AudioSpecs output_service_impl::current_specs() const {
    return m_current_specs;
}

Melosic::Output::DeviceState output_service_impl::state() const {
    return m_state;
}

bool output_service_impl::non_blocking() const {
    return m_non_blocking;
}

void output_service_impl::non_blocking(bool mode, std::error_code& ec) {
    if(m_pdh != nullptr) {
        ec = {asio::error::already_open, asio::error::misc_category};
        return;
    }
    ec = {snd_pcm_nonblock(m_pdh, mode), alsa_category};
    m_non_blocking = !ec ? true : false;
}

} // namespace alsa
