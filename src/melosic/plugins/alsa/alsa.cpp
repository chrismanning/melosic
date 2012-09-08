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

/* Use the newer ALSA API
 * TODO: Find out if this is still needed
 */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <boost/functional/factory.hpp>
using boost::factory;
#include <boost/format.hpp>
using boost::format;
#include <alsa/asoundlib.h>
#include <string>
#include <sstream>
#include <array>
#include <mutex>
#include <thread>

#include <melosic/common/common.hpp>

using namespace Melosic;

void enforceAlsaEx(int ret) {
    if(ret != 0) {
        throw Exception((str(format("ALSA: %s") % strdup(snd_strerror(ret)))).c_str());
    }
}

static const snd_pcm_format_t formats[] = {
    SND_PCM_FORMAT_S8,
    SND_PCM_FORMAT_S16_LE,
    SND_PCM_FORMAT_S24_3LE,
    SND_PCM_FORMAT_S24_LE,
    SND_PCM_FORMAT_S32_LE
};
static const uint8_t bpss[] = {8, 16, 24, 24, 32};

class AlsaOutput : public Output::IDeviceSink {
public:
    AlsaOutput(Output::OutputDeviceName name)
        : pdh(nullptr),
          params(nullptr),
          name(name.getName()),
          desc(name.getDesc()),
          state_(Output::DeviceState::Stopped)
    {}

    virtual ~AlsaOutput() {
        std::cerr << "Alsa instance being destroyed\n";
        if(pdh != nullptr) {
            stop();
        }
        if(params != nullptr)
            snd_pcm_hw_params_free(params);
    }

    virtual void prepareDevice(AudioSpecs& as) {
        std::lock_guard<Mutex> l(mu);
        current = as;
        state_ = Output::DeviceState::Error;
        if(pdh == nullptr) {
            enforceAlsaEx(snd_pcm_open(&pdh, this->name.c_str(), SND_PCM_STREAM_PLAYBACK, 0));
        }

        if(params == nullptr) {
            snd_pcm_hw_params_malloc(&params);
            snd_pcm_hw_params_any(pdh, params);
        }

        snd_pcm_hw_params_set_access(pdh, params, SND_PCM_ACCESS_RW_INTERLEAVED);

        enforceAlsaEx(snd_pcm_hw_params_set_channels(pdh, params, as.channels));
        int dir;
        unsigned rate = as.sample_rate;
        enforceAlsaEx(snd_pcm_hw_params_set_rate_near(pdh, params, &rate, &dir));
        int frames = 1024;
        snd_pcm_hw_params_set_period_size_near(pdh, params, (snd_pcm_uframes_t*)&frames, &dir);
        int buf = frames * 8;
        snd_pcm_hw_params_set_buffer_size_near(pdh, params, (snd_pcm_uframes_t*)&buf);
        enforceAlsaEx(snd_pcm_hw_params_set_rate_resample(pdh, params, false));

        snd_pcm_format_t fmt;

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

        if(snd_pcm_hw_params_test_format(pdh, params, fmt) < 0) {
            std::cerr << "unsupported format\n";
            auto p = std::find(formats, formats + sizeof(formats), fmt);
            if(p != formats + sizeof(formats)) {
                while(++p != formats + sizeof(formats)) {
                    if(!snd_pcm_hw_params_test_format(pdh, params, *p)) {
                        fmt = *p;
                        as.pad = bpss[p-formats];
                        break;
                    }
                    std::cerr << "unsupported format\n";
                }
                if(p == formats + sizeof(formats)) {
                    std::cerr << "couldnt set format\n";
                    return;
                }
            }
        }
        if(snd_pcm_hw_params_set_format(pdh, params, fmt) < 0) {
            std::cerr << "couldnt set format\n";
            return;
        }

        enforceAlsaEx(snd_pcm_hw_params(pdh, params));
//        enforceAlsaEx(snd_pcm_prepare(pdh));
        state_ = Output::DeviceState::Ready;
    }

    virtual void play() {
        std::lock_guard<Mutex> l(mu);
        if(state_ == Output::DeviceState::Stopped || state_ == Output::DeviceState::Error) {
            prepareDevice(current);
            auto s = snd_pcm_state(pdh);
            std::clog << snd_pcm_state_name(s) << std::endl;
        }
        if(state_ == Output::DeviceState::Ready) {
            enforceAlsaEx(snd_pcm_prepare(pdh));
            state_ = Output::DeviceState::Playing;
        }
        else if(state_ == Output::DeviceState::Paused) {
            unpause();
        }
    }

    virtual void pause() {
        std::lock_guard<Mutex> l(mu);
        if(state_ == Output::DeviceState::Playing) {
            if(snd_pcm_hw_params_can_pause(params)) {
                enforceAlsaEx(snd_pcm_pause(pdh, true));
            }
            else {
                enforceAlsaEx(snd_pcm_drop(pdh));
            }
            state_ = Output::DeviceState::Paused;
        }
        else if(state_ == Output::DeviceState::Paused) {
            unpause();
        }
    }

    void unpause() {
        if(snd_pcm_hw_params_can_pause(params)) {
            enforceAlsaEx(snd_pcm_pause(pdh, false));
        }
        else {
            snd_pcm_prepare(pdh);
            snd_pcm_start(pdh);
        }
        state_ = Output::DeviceState::Playing;
    }

    virtual void stop() {
        std::lock_guard<Mutex> l(mu);
        if(state_ != Output::DeviceState::Stopped && pdh) {
            enforceAlsaEx(snd_pcm_drop(pdh));
            enforceAlsaEx(snd_pcm_close(pdh));
        }
        state_ = Output::DeviceState::Stopped;
        pdh = nullptr;
    }

    virtual Output::DeviceState state() {
        std::lock_guard<Mutex> l(mu);
        return state_;
    }

    virtual std::streamsize write(const char* s, std::streamsize n) {
        std::lock_guard<Mutex> l(mu);
        if(pdh != nullptr) {
            auto frames = snd_pcm_bytes_to_frames(pdh, n);
            auto r = snd_pcm_writei(pdh, s, frames);

            if(r == -EPIPE) {
                if(pdh != nullptr) {
                    snd_pcm_recover(pdh, r, false);
                }
                return n;
            }
            else if(r < 0) {
                std::cerr << "ALSA: write error\n";
                return -1;
            }
            else if(r != frames) {
                std::cerr << "ALSA: not all frames written\n";
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

    virtual const std::string& getDeviceDescription() {
        return desc;
    }

    virtual const std::string& getSinkName() {
        return name;
    }

private:
    snd_pcm_t * pdh;
    snd_pcm_hw_params_t * params;
    AudioSpecs current;
    std::string name;
    std::string desc;
    std::mutex mu;
    typedef decltype(mu) Mutex;
    Output::DeviceState state_;
};

static std::list<AlsaOutput*> alsaPluginObjects;

extern "C" void registerPluginObjects(Kernel& k) {
    //TODO: make this more C++-like
    void ** hints, ** n;
    char * name, * desc, * io;
    std::string filter = "Output";

    enforceAlsaEx(snd_device_name_hint(-1, "pcm", &hints));

    auto& outman = k.getOutputManager();

    std::list<Output::OutputDeviceName> names;

    n = hints;
    while(*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        desc = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if(io == NULL || filter == io) {
            if(name && desc) {
                names.emplace_back(name, desc);
            }
        }
        if(name != NULL)
            free(name);
        if(desc != NULL)
            free(desc);
        if(io != NULL)
            free(io);
        n++;
    }
    snd_device_name_free_hint(hints);

    outman.addFactory(factory<std::unique_ptr<AlsaOutput>>(), names);
}

extern "C" void destroyPluginObjects() {
    std::cerr << "Destroying alsa plugin objects\n";
    for(auto output : alsaPluginObjects) {
        delete output;
    }
}
