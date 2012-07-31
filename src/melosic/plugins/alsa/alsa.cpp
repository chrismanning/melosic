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

#include <melosic/managers/common.hpp>
#include <alsa/asoundlib.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <list>

class AlsaOutput : public IOutput {
public:
    AlsaOutput(const std::string name, const std::string desc)
        : pdh(0), params(0), name(name), desc(desc)
    {}

    virtual ~AlsaOutput() {
        fprintf(stderr, "Alsa instance being destroyed\n");
        if(pdh) {
            snd_pcm_drain(pdh);
            snd_pcm_close(pdh);
        }
        if(params)
            snd_pcm_hw_params_free(params);
    }

    virtual void prepareDevice(AudioSpecs as) {
        if(snd_pcm_open(&pdh, name.c_str(), SND_PCM_STREAM_PLAYBACK, 0) < 0) {
            //FIXME: use future error handling capabilities
            fprintf(stderr, "Cannot open output device\n");
            return;
        }
        snd_pcm_hw_params_malloc(&params);
        snd_pcm_hw_params_any(pdh, params);
        snd_pcm_hw_params_set_access(pdh, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pdh, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pdh, params, as.channels);
        int dir;
        snd_pcm_hw_params_set_rate_near(pdh, params, &as.sample_rate, &dir);
        int frames = 32;
        snd_pcm_hw_params_set_period_size_near(pdh, params, (snd_pcm_uframes_t*)&frames, &dir);
        auto ret = snd_pcm_hw_params(pdh, params);
        if(ret < 0) {
            //FIXME: use future error handling capabilities
        }
//        snd_pcm_hw_params_get_period_size(params, (snd_pcm_uframes_t*)&frames, &dir);
//        buffer_size = frames * as.channels * as.bps;
//        buffer = new byte[buffer_size];
    }

    virtual void render(PlaybackHandler * playHandle) {
        if(playHandle) {
            switch(playHandle->state()) {
                case PlaybackState::Playing: {
                    fprintf(stderr, "Playing through output %s\n", desc.c_str());
                    int err, frames_to_deliver;
                    std::stringstream str;

                    // wait till the interface is ready for data, or 1 second has elapsed.
                    if((err = snd_pcm_wait(pdh, 1000)) < 0) {
                        str << "poll failed: " << strerror(errno);
                        playHandle->report(str.str().c_str());
                        return;
                    }

                    // find out how much space is available for playback data
                    if((frames_to_deliver = snd_pcm_avail_update(pdh)) < 0) {
                        if(frames_to_deliver == -EPIPE) {
                            playHandle->report("an xrun occured");
                            return;
                        }
                        else {
                            str << "unknown ALSA avail update return value: " << frames_to_deliver;
                            playHandle->report(str.str().c_str());
                            return;
                        }
                    }

                    // use some sort of streaming buffer system to only deliver whats needed
                    frames_to_deliver = frames_to_deliver > 4096 ? 4096 : frames_to_deliver;

                    unsigned framesize;
                    snd_pcm_hw_params_get_channels(params, &framesize);

                    framesize *= 2/*change this for !16 bps*/;

                    auto tmp = playHandle->requestData(frames_to_deliver * framesize);

                    if((err = snd_pcm_writei(pdh, tmp->ptr(), tmp->length() / framesize)) < 0) {
                        str << "write failed: " << snd_strerror(err);
                        playHandle->report(str.str().c_str());
                    }
                    break;
                }
                case PlaybackState::Paused: {
                    snd_pcm_drop(pdh);
                    break;
                }
                case PlaybackState::Stopped: {
                    snd_pcm_close(pdh);
                    break;
                }
            }
        }
    }

    virtual const std::string& getDeviceDescription() {
        return desc;
    }

    virtual const std::string& getDeviceName() {
        return name;
    }

    virtual const DeviceCapabilities * getDeviceCapabilities() {
        return new DeviceCapabilities;
    }

private:
    snd_pcm_t * pdh;
    snd_pcm_hw_params_t * params;
    const std::string name;
    const std::string desc;
};

static std::list<AlsaOutput*> alsaPluginObjects;

extern "C" void registerPluginObjects(IKernel * k) {
    //TODO: make this more C++-like
    void ** hints, ** n;
    char * name, * desc, * io;
    const char * filter = "Output";
    AlsaOutput * tmp;
    if(snd_device_name_hint(-1, "pcm", &hints) < 0)
        return;
    n = hints;
    while(*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        desc = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if(io == NULL || !strcmp(io, filter)) {
            if(name && desc) {
                tmp = new AlsaOutput(name, desc);
                alsaPluginObjects.push_back(tmp);
                k->getOutputManager()->addOutput(tmp);
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
}

extern "C" void destroyPluginObjects() {
    fprintf(stderr, "Destroying alsa plugin objects\n");
    for(auto output : alsaPluginObjects) {
        delete output;
    }
}
