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
#include <boost/asio/posix/stream_descriptor.hpp>

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

static constexpr Plugin::Info alsaInfo("ALSA",
                                       Plugin::Type::outputDevice,
                                       Plugin::Version(1,0,0));

#define ALSA_THROW_IF(Exc, ret) if(ret != 0) {\
    BOOST_THROW_EXCEPTION(Exc() << ErrorTag::Output::DeviceErrStr(strdup(snd_strerror(ret)))\
    << ErrorTag::Plugin::Info(::alsaInfo));\
}

static constexpr std::array<snd_pcm_format_t, 5> formats{{
    SND_PCM_FORMAT_S8,
    SND_PCM_FORMAT_S16_LE,
    SND_PCM_FORMAT_S24_3LE,
    SND_PCM_FORMAT_S24_LE,
    SND_PCM_FORMAT_S32_LE
}};
static constexpr std::array<uint8_t, 5> bpss{{8, 16, 24, 24, 32}};

Config::Conf conf{"ALSA"};
snd_pcm_uframes_t frames = 1024;
bool resample = false;

struct alsa_category_ : boost::system::error_category {
    const char* name() const noexcept override {
        return "alsa";
    }

    std::string message(int errnum) const override {
        return snd_strerror(errnum);
    }
} alsa_category;

struct MELOSIC_EXPORT AlsaOutputServiceImpl : ASIO::AudioOutputServiceBase {
    explicit AlsaOutputServiceImpl(ASIO::io_service& service) :
        ASIO::AudioOutputServiceBase(service),
        m_asio_fd(service)
    {}

    void assign(Output::DeviceName dev_name, boost::system::error_code& ec) noexcept override {
        if(m_pdh != nullptr) {
            ec = {ASIO::error::already_open, ASIO::error::get_misc_category()};
            return;
        }
        m_name = dev_name.getName();
        m_desc = dev_name.getDesc();
    }

    void destroy() override {
        boost::system::error_code ec;
        if(m_pdh != nullptr)
            stop(ec);
        if(m_params != nullptr)
            snd_pcm_hw_params_free(m_params);
    }

    void cancel(boost::system::error_code& ec) noexcept override {
        if(m_asio_fd.is_open())
            m_asio_fd.cancel(ec);
    }

    void prepare(AudioSpecs& as, boost::system::error_code& ec) noexcept override {
        m_current_specs = as;
        m_state = Output::DeviceState::Error;
        if((m_pdh == nullptr) &&
                (ec = {snd_pcm_open(&m_pdh, m_name.c_str(), SND_PCM_STREAM_PLAYBACK, m_non_blocking), alsa_category}))
            return;

        if((m_params == nullptr) && (ec = {snd_pcm_hw_params_malloc(&m_params), alsa_category}))
            return;

        snd_pcm_hw_params_any(m_pdh, m_params);

        snd_pcm_hw_params_set_access(m_pdh, m_params, SND_PCM_ACCESS_RW_INTERLEAVED);

        if((ec = {snd_pcm_hw_params_set_channels(m_pdh, m_params, as.channels), alsa_category}))
            return;
        int dir = 0;
        if((ec = {snd_pcm_hw_params_set_rate_resample(m_pdh, m_params, ::resample), alsa_category}))
            return;
        unsigned rate = as.sample_rate;
        if((ec = {snd_pcm_hw_params_set_rate_near(m_pdh, m_params, &rate, &dir), alsa_category}))
            return;
        if(rate != as.sample_rate) {
            WARN_LOG(logject) << "Sample rate: " << as.sample_rate << " not supported. Using " << rate;
            as.target_sample_rate = rate;
        }

        auto frames = ::frames;
        if((ec = {snd_pcm_hw_params_set_period_size_near(m_pdh, m_params, &frames, &dir), alsa_category}))
            return;
        auto buf = frames * 8;
        if((ec = {snd_pcm_hw_params_set_buffer_size_near(m_pdh, m_params, &buf), alsa_category}))
            return;

        snd_pcm_format_t fmt(SND_PCM_FORMAT_UNKNOWN);

        switch(as.bps) {
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
        }
        as.target_bps = as.bps;

        if(snd_pcm_hw_params_test_format(m_pdh, m_params, fmt) < 0) {
            WARN_LOG(logject) << "unsupported format";

            auto p = std::find(formats.begin(), formats.end(), fmt);
            auto bps = &bpss[std::distance(formats.begin(), p)];
            for(; p < formats.end(); p++, bps++) {
                WARN_LOG(logject) << "trying higher bps of " << static_cast<unsigned>(*bps);
                if(!snd_pcm_hw_params_test_format(m_pdh, m_params, *p)) {
                    fmt = *p;
                    as.target_bps = *bps;
                    WARN_LOG(logject) << "settled on bps of " << static_cast<unsigned>(as.target_bps);
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
                        as.target_bps = *bps;
                        WARN_LOG(logject) << "settled on bps of " << static_cast<unsigned>(as.target_bps);
                        break;
                    }
                }
                if(rp == formats.rend()) {
                    ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
                    return;
                }
            }
        }

        if((ec = {snd_pcm_hw_params_set_format(m_pdh, m_params, fmt), alsa_category}))
            return;

        if((ec = {snd_pcm_hw_params(m_pdh, m_params), alsa_category}))
            return;

        m_current_specs = as;

        auto pcount = snd_pcm_poll_descriptors_count(m_pdh);
        if((pcount < 0) && (ec = {pcount, alsa_category}))
            return;

        m_pfds.clear();
        m_pfds.resize(pcount);

        auto n = snd_pcm_poll_descriptors(m_pdh, m_pfds.data(), m_pfds.size());
        TRACE_LOG(logject) << "no. poll descriptors: " << m_pfds.size();
        TRACE_LOG(logject) << "no. poll descriptors filled: " << n;
        assert(!m_pfds.empty());
        if((n < 0) && (ec = {n, alsa_category}))
            return;

        m_mangled_revents = static_cast<bool>(m_pfds.data()->events & POLLIN);

        if(m_asio_fd.is_open()) {
            m_asio_fd.cancel();
            m_asio_fd.close();
        }
        if(m_asio_fd.assign(m_pfds.data()->fd, ec)) return;

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
    }

    void play(boost::system::error_code& ec) noexcept override {
        switch(m_state) {
            case Output::DeviceState::Stopped:
            case Output::DeviceState::Error:
                prepare(m_current_specs, ec);
                if(ec)
                    return;
                break;
            case Output::DeviceState::Ready:
                if((ec = {snd_pcm_prepare(m_pdh), alsa_category})) return;
                m_state = Output::DeviceState::Playing;
                break;
            case Output::DeviceState::Paused:
                unpause(ec);
                if(ec) return;
                break;
            default:
                break;
        }
        assert(!ec);
    }

    void pause(boost::system::error_code& ec) noexcept override {
        if(m_state == Output::DeviceState::Playing) {
            if(snd_pcm_hw_params_can_pause(m_params)) {
                if((ec = {snd_pcm_pause(m_pdh, true), alsa_category})) return;
            }
            else {
                if((ec = {snd_pcm_drop(m_pdh), alsa_category})) return;
            }
            cancel(ec);
            m_state = Output::DeviceState::Paused;
        }
        else if(m_state == Output::DeviceState::Paused) {
            unpause(ec);
            if(ec) return;
        }
        assert(!ec);
    }

    void unpause(boost::system::error_code& ec) noexcept override {
        if(snd_pcm_hw_params_can_pause(m_params)) {
            if((ec = {snd_pcm_pause(m_pdh, false), alsa_category})) return;
        }
        else {
            snd_pcm_prepare(m_pdh);
            snd_pcm_start(m_pdh);
        }
        m_state = Output::DeviceState::Playing;
        assert(!ec);
    }

    void stop(boost::system::error_code& ec) noexcept override {
        if(m_state != Output::DeviceState::Stopped && m_state != Output::DeviceState::Error && m_pdh) {
            cancel(ec);
            if((ec = {snd_pcm_drop(m_pdh), alsa_category})) return;
            if((ec = {snd_pcm_close(m_pdh), alsa_category})) return;
        }
        m_pfds.clear();
        m_asio_fd.release();
        m_state = Output::DeviceState::Stopped;
        m_pdh = nullptr;
        assert(!ec);
    }

    size_t write_some(const ASIO::const_buffer& buf, boost::system::error_code& ec) noexcept override {
        if(m_pdh == nullptr)
            BOOST_THROW_EXCEPTION(DeviceWriteException());

        if(ec)
            return 0;
        const auto size = ASIO::buffer_size(buf);
        auto ptr = ASIO::buffer_cast<const char*>(buf);

        auto frames = snd_pcm_bytes_to_frames(m_pdh, size);
        auto r = snd_pcm_writei(m_pdh, ptr, frames);

        if(r < 0) {
            if(m_pdh != nullptr)
                r = snd_pcm_recover(m_pdh, r, false);
            ec = {static_cast<int>(r), alsa_category};
            return 0;
        }
        else if(r != frames)
            WARN_LOG(logject) << "not all frames written";

        assert(m_pdh != nullptr);
        r = snd_pcm_frames_to_bytes(m_pdh, r);

        return r;
    }

    void asyncPrepare(AudioSpecs&, boost::system::error_code&) noexcept override {
    }

    void async_write_some(const ASIO::const_buffer& buf, WriteHandler handler) override {
        auto f = get_strand().wrap([=] (boost::system::error_code ec, std::size_t n) {
            if(!ec)
                n = write_some(buf, ec);
            handler(ec, n);
        });

        if(m_mangled_revents)
            m_asio_fd.async_read_some(ASIO::null_buffers(), f);
        else
            m_asio_fd.async_write_some(ASIO::null_buffers(), f);
    }

    const AudioSpecs& currentSpecs() const noexcept override {
        return m_current_specs;
    }

    Output::DeviceState state() const noexcept override {
        return m_state;
    }

    bool non_blocking() const noexcept override {
        return m_non_blocking;
    }
    void non_blocking(bool mode, boost::system::error_code& ec) noexcept override {
        if(m_pdh != nullptr) {
            ec = {ASIO::error::already_open, ASIO::error::misc_category};
            return;
        }
        ec = {snd_pcm_nonblock(m_pdh, mode), alsa_category};
        m_non_blocking = !ec ? true : false;
    }

    snd_pcm_t* m_pdh = nullptr;
    snd_pcm_hw_params_t* m_params = nullptr;
    std::vector<pollfd> m_pfds;
    ASIO::posix::stream_descriptor m_asio_fd;
    bool m_non_blocking = true;
    bool m_mangled_revents = true;
    AudioSpecs m_current_specs;
    std::string m_name{"default"};
    std::string m_desc;
    Output::DeviceState m_state;
};

typedef ASIO::AudioOutputService<AlsaOutputServiceImpl> AlsaOutputService;
typedef ASIO::BasicAudioOutput<AlsaOutputService> AlsaAsioOutput;

extern "C" MELOSIC_EXPORT void registerOutput(Output::Manager* outman) {
    //TODO: make this more C++-like
    void ** hints, ** n;
    char * name, * desc, * io;
    const std::string filter = "Output";

    ALSA_THROW_IF(DeviceOpenException, snd_device_name_hint(-1, "pcm", &hints));

    std::list<Output::DeviceName> names;

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

    outman->addOutputDevices([] (ASIO::io_service& io_service, Output::DeviceName name) {
        return std::make_unique<AlsaAsioOutput>(io_service, name);
    }, names);
}

void variableUpdateSlot(const Config::KeyType& key, const Config::VarType& val) {
    using boost::get;
    using Melosic::Config::get;
    try {
        if(key == "frames")
            ::frames = get<snd_pcm_uframes_t>(val);
        else if(key == "resample")
            ::resample = get<bool>(val);
    }
    catch(boost::bad_get& e) {
        ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
        TRACE_LOG(logject) << e.what();
    }
}

void loadedSlot(Config::Conf& base, std::unique_lock<std::mutex>& l) {
    assert(l);
    auto o = base.getChild("Output"s);
    if(!o) {
        base.putChild(Config::Conf("Output"s));
        o = base.getChild("Output"s);
    }
    assert(o);
    auto c = o->getChild("ALSA"s);
    if(!c) {
        o->putChild(::conf);
        c = o->getChild("ALSA"s);
    }
    assert(c);
    c->merge(::conf);
    c->addDefaultFunc([=]() -> Config::Conf { return ::conf; });
    c->getVariableUpdatedSignal().connect(variableUpdateSlot);
    c->iterateNodes([&] (const std::pair<Config::KeyType, Config::VarType>& pair) {
        TRACE_LOG(logject) << "Config: variable loaded: " << pair.first;
        variableUpdateSlot(pair.first, pair.second);
    });
}

extern "C" MELOSIC_EXPORT void registerConfig(Config::Manager* confman) {
    ::conf.putNode("frames", ::frames);
    ::conf.putNode("resample", ::resample);
    std::reference_wrapper<Config::Conf> base{::conf};
    std::unique_lock<std::mutex> l;
    std::tie(base, l) = confman->getConfigRoot();
    loadedSlot(base, l);
}

extern "C" MELOSIC_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::alsaInfo;
    funs << registerOutput << registerConfig;
}

extern "C" MELOSIC_EXPORT void destroyPlugin() {
}
