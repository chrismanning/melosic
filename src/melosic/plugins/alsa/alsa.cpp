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

#include <alsa/asoundlib.h>
#include <poll.h>

#include <string>
#include <memory>

#include <boost/variant.hpp>
#include <boost/config.hpp>
#include <asio/posix/stream_descriptor.hpp>

#include <melosic/common/error.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/exports.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/int_get.hpp>
#include <melosic/asio/audio_impl.hpp>
#include <melosic/asio/audio_io.hpp>
using namespace Melosic;

Logger::Logger logject(logging::keywords::channel = "ALSA");

Plugin::Info alsaInfo("ALSA", Plugin::Type::outputDevice, Plugin::Version(1, 0, 0));

#define ALSA_THROW_IF(Exc, ret)                                                                                        \
    if(ret != 0) {                                                                                                     \
        BOOST_THROW_EXCEPTION(Exc() << ErrorTag::Output::DeviceErrStr(strdup(snd_strerror(ret)))                       \
                                    << ErrorTag::Plugin::Info(::alsaInfo));                                            \
    }

static constexpr std::array<snd_pcm_format_t, 5> formats{
    {SND_PCM_FORMAT_S8, SND_PCM_FORMAT_S16_LE, SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S24_LE, SND_PCM_FORMAT_S32_LE}};
static constexpr std::array<uint8_t, 5> bpss{{8, 16, 24, 24, 32}};

Config::Conf conf{"ALSA"};
snd_pcm_uframes_t frames = 1024;
bool resample = false;
chrono::milliseconds buf_time_msecs{1000ms};

struct alsa_category_ : std::error_category {
    const char* name() const noexcept override {
        return "alsa";
    }

    std::string message(int errnum) const override {
        return snd_strerror(errnum);
    }
} alsa_category;

struct MELOSIC_EXPORT AlsaOutputServiceImpl : AudioIO::AudioOutputServiceBase {
    explicit AlsaOutputServiceImpl(asio::io_service& service)
        : AudioIO::AudioOutputServiceBase(service), m_asio_fd(service) {
    }

    void assign(Output::device_descriptor dev_name, std::error_code& ec) override {
        if(m_pdh != nullptr) {
            ec = {asio::error::already_open, asio::error::get_misc_category()};
            return;
        }
        m_name = dev_name.getName();
        m_desc = dev_name.getDesc();
    }

    void destroy() override {
        std::error_code ec;
        if(m_pdh != nullptr)
            stop(ec);
        if(m_params != nullptr)
            snd_pcm_hw_params_free(m_params);
    }

    void cancel(std::error_code& ec) override {
        if(m_asio_fd.is_open())
            m_asio_fd.cancel(ec);
    }

    AudioSpecs prepare(AudioSpecs as, std::error_code& ec) override {
        m_current_specs = as;
        m_state = Output::DeviceState::Error;
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
        if((ec = {snd_pcm_hw_params_set_rate_resample(m_pdh, m_params, ::resample), alsa_category}))
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

        m_state = Output::DeviceState::Ready;
        assert(!ec);
        TRACE_LOG(logject) << "internal state after prepare: " << snd_pcm_state_name(snd_pcm_state(m_pdh));

        return m_current_specs;
    }

    void play(std::error_code& ec) override {
        switch(m_state) {
            case Output::DeviceState::Stopped:
            case Output::DeviceState::Error:
                prepare(m_current_specs, ec);
                if(ec)
                    return;
                break;
            case Output::DeviceState::Ready:
                if((ec = {snd_pcm_prepare(m_pdh), alsa_category}))
                    return;
                m_state = Output::DeviceState::Playing;
                break;
            case Output::DeviceState::Paused:
                unpause(ec);
                if(ec)
                    return;
                break;
            default:
                break;
        }
        assert(!ec);
    }

    void pause(std::error_code& ec) override {
        if(m_state == Output::DeviceState::Playing) {
            if(snd_pcm_hw_params_can_pause(m_params)) {
                if((ec = {snd_pcm_pause(m_pdh, true), alsa_category}))
                    return;
            } else {
                if((ec = {snd_pcm_drop(m_pdh), alsa_category}))
                    return;
            }
            cancel(ec);
            m_state = Output::DeviceState::Paused;
        } else if(m_state == Output::DeviceState::Paused) {
            unpause(ec);
            if(ec)
                return;
        }
        assert(!ec);
    }

    void unpause(std::error_code& ec) override {
        if(snd_pcm_hw_params_can_pause(m_params)) {
            if((ec = {snd_pcm_pause(m_pdh, false), alsa_category}))
                return;
        } else {
            snd_pcm_prepare(m_pdh);
            snd_pcm_start(m_pdh);
        }
        m_state = Output::DeviceState::Playing;
        assert(!ec);
    }

    void stop(std::error_code& ec) override {
        if(m_state != Output::DeviceState::Stopped && m_state != Output::DeviceState::Error && m_pdh) {
            cancel(ec);
            if((ec = {snd_pcm_drop(m_pdh), alsa_category}))
                return;
            if((ec = {snd_pcm_close(m_pdh), alsa_category}))
                return;
        }
        m_pfds.clear();
        m_asio_fd.release();
        m_state = Output::DeviceState::Stopped;
        m_pdh = nullptr;
        assert(!ec);
    }

    size_t write_some(const asio::const_buffer& buf, std::error_code& ec) override {
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

    void async_prepare(const AudioSpecs, PrepareHandler) override {
    }

    void async_write_some(const asio::const_buffer& buf, WriteHandler handler) override {
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

    AudioSpecs current_specs() const override {
        return m_current_specs;
    }

    Output::DeviceState state() const override {
        return m_state;
    }

    bool non_blocking() const override {
        return m_non_blocking;
    }

    void non_blocking(bool mode, std::error_code& ec) override {
        if(m_pdh != nullptr) {
            ec = {asio::error::already_open, asio::error::misc_category};
            return;
        }
        ec = {snd_pcm_nonblock(m_pdh, mode), alsa_category};
        m_non_blocking = !ec ? true : false;
    }

    snd_pcm_t* m_pdh = nullptr;
    snd_pcm_hw_params_t* m_params = nullptr;
    std::vector<pollfd> m_pfds;
    asio::posix::stream_descriptor m_asio_fd;
    bool m_non_blocking = true;
    bool m_mangled_revents = true;
    AudioSpecs m_current_specs;
    std::string m_name{"default"};
    std::string m_desc;
    Output::DeviceState m_state;
};

typedef AudioIO::AudioOutputService<AlsaOutputServiceImpl> AlsaOutputService;
typedef AudioIO::BasicAudioOutput<AlsaOutputService> AlsaAsioOutput;

extern "C" MELOSIC_EXPORT void registerOutput(Output::Manager* outman) {
    // TODO: make this more C++-like
    void** hints, **n;
    char* name, *desc, *io;
    const std::string filter = "Output";

    ALSA_THROW_IF(DeviceOpenException, snd_device_name_hint(-1, "pcm", &hints));

    std::list<Output::device_descriptor> names;

    n = hints;
    TRACE_LOG(logject) << "Enumerating output devices";
    while(*n != nullptr) {
        name = snd_device_name_get_hint(*n, "NAME");
        desc = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if((io == nullptr || filter == io) && (name && desc)) {
            names.emplace_back(name, desc);
            TRACE_LOG(logject) << names.back();
        }
        if(name != nullptr)
            free(name);
        if(desc != nullptr)
            free(desc);
        if(io != nullptr)
            free(io);
        n++;
    }
    snd_device_name_free_hint(hints);

    outman->addOutputDevices([](asio::io_service& io_service, Output::device_descriptor name) {
        return std::make_unique<AlsaAsioOutput>(io_service, name);
    }, names);
}

void variableUpdateSlot(const std::string& key, const Config::VarType& val) {
    TRACE_LOG(logject) << "Config: variable updated: " << key;
    using Melosic::Config::get;
    try {
        if(key == "frames")
            ::frames = get<snd_pcm_uframes_t>(val);
        else if(key == "resample")
            ::resample = get<bool>(val);
        else if(key == "buffer time")
            ::buf_time_msecs = chrono::milliseconds(get<uint64_t>(val));
    } catch(boost::bad_get& e) {
        ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        TRACE_LOG(logject) << e.what();
    }
}

std::vector<Signals::ScopedConnection> g_signal_connections;

void loadedSlot(Config::Conf& base) {
    auto output_conf = base.createChild("Output"s);

    output_conf->iterateNodes([&](const std::string& key, auto&& var) { variableUpdateSlot(key, var); });

    auto c = output_conf->createChild("ALSA"s, ::conf);

    c->merge(::conf);
    c->setDefault(::conf);

    c->iterateNodes([&](const std::string& key, auto&& var) {
        TRACE_LOG(logject) << "Config: variable loaded: " << key;
        variableUpdateSlot(key, var);
    });

    g_signal_connections.emplace_back(c->getVariableUpdatedSignal().connect(variableUpdateSlot));
}

extern "C" BOOST_SYMBOL_EXPORT void registerConfig(Config::Manager* confman) {
    ::conf.putNode("frames", ::frames);
    ::conf.putNode("resample", ::resample);
    auto base = confman->getConfigRoot().synchronize();
    loadedSlot(*base);
}

//extern "C" BOOST_SYMBOL_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
//    *info = ::alsaInfo;
//    funs << registerOutput << registerConfig;
//}

extern "C" BOOST_SYMBOL_EXPORT void destroyPlugin() {
}
