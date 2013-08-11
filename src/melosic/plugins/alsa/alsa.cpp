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

#include <thread>
#include <mutex>
using std::unique_lock; using std::lock_guard;
#include <string>
#include <memory>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
using boost::shared_mutex; using boost::shared_lock_guard;
#include <boost/variant.hpp>

#include <melosic/common/error.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/common.hpp>
#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/exports.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/config.hpp>
using namespace Melosic;

Logger::Logger logject(logging::keywords::channel = "ALSA");

static constexpr Plugin::Info alsaInfo("ALSA",
                                       Plugin::Type::outputDevice,
                                       Plugin::Version(1,0,0));

#define ALSA_THROW_IF(Exc, ret) if(ret != 0) {\
    BOOST_THROW_EXCEPTION(Exc() << ErrorTag::Output::DeviceErrStr(strdup(snd_strerror(ret)))\
    << ErrorTag::Plugin::Info(::alsaInfo));\
}

static constexpr snd_pcm_format_t formats[] = {
    SND_PCM_FORMAT_S8,
    SND_PCM_FORMAT_S16_LE,
    SND_PCM_FORMAT_S24_3LE,
    SND_PCM_FORMAT_S24_LE,
    SND_PCM_FORMAT_S32_LE
};
static constexpr uint8_t bpss[] = {8, 16, 24, 24, 32};

Config::Conf conf{"ALSA"};
int64_t frames = 0;

class AlsaOutput : public Output::PlayerSink {
public:
    explicit AlsaOutput(Output::DeviceName name)
        : pdh(nullptr),
          params(nullptr),
          name(name.getName()),
          desc(name.getDesc()),
          state_(Output::DeviceState::Stopped)
    {}

    virtual ~AlsaOutput() {
        if(pdh != nullptr) {
            stop();
        }
        if(params != nullptr)
            snd_pcm_hw_params_free(params);
    }

    void prepareSink(AudioSpecs& as) override {
        lock_guard<Mutex> l(mu);
        current = as;
        state_ = Output::DeviceState::Error;
        if(pdh == nullptr) {
            ALSA_THROW_IF(DeviceOpenException, snd_pcm_open(&pdh, this->name.c_str(), SND_PCM_STREAM_PLAYBACK, 0));
        }

        if(params == nullptr) {
            snd_pcm_hw_params_malloc(&params);
        }
        snd_pcm_hw_params_any(pdh, params);

        snd_pcm_hw_params_set_access(pdh, params, SND_PCM_ACCESS_RW_INTERLEAVED);

        ALSA_THROW_IF(DeviceParamException, snd_pcm_hw_params_set_channels(pdh, params, as.channels));
        int dir = 0;
        ALSA_THROW_IF(DeviceParamException, snd_pcm_hw_params_set_rate_resample(pdh, params, false));
        unsigned rate = as.sample_rate;
        ALSA_THROW_IF(DeviceParamException, snd_pcm_hw_params_set_rate_near(pdh, params, &rate, &dir));
        if(rate != as.sample_rate) {
            WARN_LOG(logject) << "Sample rate: " << as.sample_rate << " not supported. Using " << rate;
            as.target_sample_rate = rate;
        }

        const int frames = ::frames;
        snd_pcm_hw_params_set_period_size_near(pdh, params, (snd_pcm_uframes_t*)&frames, &dir);
        const int buf = frames * 8;
        snd_pcm_hw_params_set_buffer_size_near(pdh, params, (snd_pcm_uframes_t*)&buf);

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

        if(snd_pcm_hw_params_test_format(pdh, params, fmt) < 0) {
            WARN_LOG(logject) << "unsupported format";
            auto p = std::find(formats, formats + sizeof(formats), fmt);
            if(p != formats + sizeof(formats)) {
                while(++p != formats + sizeof(formats)) {
                    WARN_LOG(logject) << "trying higher bps";
                    if(!snd_pcm_hw_params_test_format(pdh, params, *p)) {
                        fmt = *p;
                        as.target_bps = bpss[p-formats];
                        break;
                    }
                    WARN_LOG(logject) << "unsupported format";
                }
                if(p == formats + sizeof(formats)) {
                    ERROR_LOG(logject) << "couldnt set format";
                    return;
                }
            }
        }
        if(snd_pcm_hw_params_set_format(pdh, params, fmt) < 0) {
            ERROR_LOG(logject) << "couldnt set format";
            return;
        }

        ALSA_THROW_IF(DeviceParamException, snd_pcm_hw_params(pdh, params));

        current = as;

        state_ = Output::DeviceState::Ready;
    }

    const Melosic::AudioSpecs& currentSpecs() override {
        return current;
    }

    void play() override {
        unique_lock<Mutex> l(mu);
        if(state_ == Output::DeviceState::Stopped || state_ == Output::DeviceState::Error) {
            l.unlock();
            prepareSink(current);
            l.lock();
        }
        if(state_ == Output::DeviceState::Ready) {
            ALSA_THROW_IF(DeviceOpenException, snd_pcm_prepare(pdh));
            state_ = Output::DeviceState::Playing;
        }
        else if(state_ == Output::DeviceState::Paused) {
            unpause();
        }
    }

    void pause() override {
        lock_guard<Mutex> l(mu);
        if(state_ == Output::DeviceState::Playing) {
            if(snd_pcm_hw_params_can_pause(params)) {
                ALSA_THROW_IF(DeviceException, snd_pcm_pause(pdh, true));
            }
            else {
                ALSA_THROW_IF(DeviceException, snd_pcm_drop(pdh));
            }
            state_ = Output::DeviceState::Paused;
        }
        else if(state_ == Output::DeviceState::Paused) {
            unpause();
        }
    }

    void unpause() {
        if(snd_pcm_hw_params_can_pause(params)) {
            ALSA_THROW_IF(DeviceException, snd_pcm_pause(pdh, false));
        }
        else {
            snd_pcm_prepare(pdh);
            snd_pcm_start(pdh);
        }
        state_ = Output::DeviceState::Playing;
    }

    void stop() override {
        lock_guard<Mutex> l(mu);
        if(state_ != Output::DeviceState::Stopped && state_ != Output::DeviceState::Error && pdh) {
            ALSA_THROW_IF(DeviceException, snd_pcm_drop(pdh));
            ALSA_THROW_IF(DeviceException, snd_pcm_close(pdh));
        }
        state_ = Output::DeviceState::Stopped;
        pdh = nullptr;
    }

    Output::DeviceState state() override {
        shared_lock_guard<Mutex> l(mu);
        return state_;
    }

    std::streamsize write(const char* s, std::streamsize n) override {
        unique_lock<Mutex> l(mu);
        if(pdh != nullptr) {
            auto frames = snd_pcm_bytes_to_frames(pdh, n);
//            l.unlock();
            auto r = snd_pcm_writei(pdh, s, frames);
//            l.lock();

            if(r == -EPIPE) {
                if(pdh != nullptr) {
                    snd_pcm_recover(pdh, r, false);
                }
                return n;
            }
            else if(r < 0) {
                ERROR_LOG(logject) << "ALSA: write error";
                ALSA_THROW_IF(DeviceWriteException, r);
            }
            else if(r != frames) {
                ERROR_LOG(logject) << "ALSA: not all frames written";
            }

            if(pdh != nullptr) {
                r = snd_pcm_frames_to_bytes(pdh, r);
            }

            return r;
        }
        else {
            return -1;
        }
    }

    const std::string& getSinkDescription() override {
        return desc;
    }

    const std::string& getSinkName() override {
        return name;
    }

private:
    snd_pcm_t * pdh;
    snd_pcm_hw_params_t * params;
    AudioSpecs current;
    std::string name;
    std::string desc;
    typedef shared_mutex Mutex;
    Mutex mu;
    Output::DeviceState state_;
};

extern "C" MELOSIC_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::alsaInfo;
    funs << registerOutput << registerConfig;
}

extern "C" MELOSIC_EXPORT void registerOutput(Output::Manager* outman) {
    //TODO: make this more C++-like
    void ** hints, ** n;
    char * name, * desc, * io;
    std::string filter = "Output";

    ALSA_THROW_IF(DeviceOpenException, snd_device_name_hint(-1, "pcm", &hints));

    std::list<Output::DeviceName> names;

    n = hints;
    TRACE_LOG(logject) << "Enumerating output devices";
    while(*n != nullptr) {
        name = snd_device_name_get_hint(*n, "NAME");
        desc = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if(io == nullptr || filter == io) {
            if(name && desc) {
                names.emplace_back(name, desc);
                TRACE_LOG(logject) << names.back();
            }
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

    outman->addOutputDevices([] (Output::DeviceName name) { return std::make_unique<AlsaOutput>(name); }, names);
}

extern "C" MELOSIC_EXPORT void registerConfig(Config::Manager* confman) {
    ::conf.putNode("frames", static_cast<int64_t>(1024));
    confman->getLoadedSignal().connect([](Config::Conf& base) {
        auto& c = base.existsChild("Output")
                ? base.getChild("Output").existsChild("ALSA")
                  ? base.getChild("Output").getChild("ALSA")
                  : base.getChild("Output").putChild("ALSA", Config::Conf("ALSA"))
                  : base.putChild("Output", Config::Conf("Output")).putChild("ALSA", Config::Conf("ALSA"));
        c.merge(::conf);
        c.addDefaultFunc([=]() -> Config::Conf { return ::conf; });
        if(!c.existsNode("frames") || !c.existsNode("output device"))
            c.resetToDefault();
        auto& varUpdate = c.getVariableUpdatedSignal();
        auto fun = [&](const std::string& k, const Config::VarType& v) {
            try {
                if(k == "frames")
                    ::frames = boost::get<int64_t>(v);
            }
            catch(boost::bad_get&) {
                ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << k;
            }
        };
        varUpdate.connect(fun);
        c.iterateNodes([&] (const Config::Conf::NodeMap::value_type& node) {
            TRACE_LOG(logject) << "Config: variable loaded: " << node.first;
            fun(node.first, node.second);
        });
    });
}

extern "C" MELOSIC_EXPORT void destroyPlugin() {
}
