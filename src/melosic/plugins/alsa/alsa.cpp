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
#include <list>

#include <melosic/common/common.hpp>
using namespace Melosic;

class AlsaException : public Exception {
public:
    AlsaException(int err) : Exception((str(format("ALSA: %s") % strdup(snd_strerror(err)))).c_str()) {}
};

class AlsaOutput : public Output::IDeviceSink {
public:
    AlsaOutput(Output::OutputDeviceName name)
        : pdh(0), params(0), name(name.getName()), desc(name.getDesc())
    {
        auto ret = snd_pcm_open(&pdh, this->name.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
        enforceEx<Exception, AlsaException>(ret == 0, ret);
        snd_pcm_hw_params_malloc(&params);
        snd_pcm_hw_params_any(pdh, params);
    }

    virtual ~AlsaOutput() {
        std::cerr << "Alsa instance being destroyed\n";
        if(pdh) {
            snd_pcm_drain(pdh);
            snd_pcm_close(pdh);
        }
        if(params)
            snd_pcm_hw_params_free(params);
    }

    virtual void prepareDevice(AudioSpecs as) {
        int ret;
        snd_pcm_hw_params_set_access(pdh, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pdh, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pdh, params, as.channels);
        int dir;
        snd_pcm_hw_params_set_rate_near(pdh, params, &as.sample_rate, &dir);
        int frames = 32;
        snd_pcm_hw_params_set_period_size_near(pdh, params, (snd_pcm_uframes_t*)&frames, &dir);
        ret = snd_pcm_hw_params(pdh, params);
        enforceEx<Exception, AlsaException>(ret == 0, ret);
    }

    virtual void render(PlaybackHandler * playHandle) {
        if(playHandle) {
            switch(playHandle->state()) {
                case PlaybackState::Playing: {
                    std::clog << "Playing through output: " << getDeviceDescription() << std::endl;
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

    virtual const std::string& getSinkName() {
        return name;
    }

private:
    snd_pcm_t * pdh;
    snd_pcm_hw_params_t * params;
    std::string name;
    std::string desc;
};

static std::list<AlsaOutput*> alsaPluginObjects;

extern "C" void registerPluginObjects(Kernel& k) {
    //TODO: make this more C++-like
    void ** hints, ** n;
    char * name, * desc, * io;
    std::string filter = "Output";

    auto ret = snd_device_name_hint(-1, "pcm", &hints);
    enforceEx<Exception, AlsaException>(ret == 0, ret);

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
