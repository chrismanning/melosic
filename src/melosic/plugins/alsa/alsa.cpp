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

#include <string>
#include <memory>

#include <boost/variant.hpp>
#include <boost/config.hpp>

#include <melosic/melin/logging.hpp>
#include <melosic/melin/exports.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/common/signal.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/common/int_get.hpp>
using namespace Melosic;

#include "alsa.hpp"
#include "alsa_provider.hpp"

namespace alsa {

Logger::Logger logject(logging::keywords::channel = "ALSA");

const Plugin::Info alsaInfo("ALSA", Plugin::Type::outputDevice, Plugin::Version(1, 0, 0));

Plugin::Info plugin_info() {
    return alsaInfo;
}
MELOSIC_DLL_TYPED_ALIAS(plugin_info)

Output::provider* output_provider() {
    return new provider;
}
MELOSIC_DLL_TYPED_ALIAS(output_provider)

//void variableUpdateSlot(const std::string& key, const Config::VarType& val) {
//    TRACE_LOG(logject) << "Config: variable updated: " << key;
//    using Melosic::Config::get;
//    try {
//        if(key == "frames")
//            frames = get<snd_pcm_uframes_t>(val);
//        else if(key == "resample")
//            resample = get<bool>(val);
//        else if(key == "buffer time")
//            buf_time_msecs = chrono::milliseconds(get<uint64_t>(val));
//    } catch(boost::bad_get& e) {
//        ERROR_LOG(logject) << "Config: Couldn't get variable for key: " << key;
//        TRACE_LOG(logject) << e.what();
//    }
//}

//std::vector<Signals::ScopedConnection> g_signal_connections;

//void loadedSlot(Config::Conf& base) {
//    auto output_conf = base.createChild("Output"s);

//    output_conf->iterateNodes([&](const std::string& key, auto&& var) { variableUpdateSlot(key, var); });

//    auto c = output_conf->createChild("ALSA"s, conf);

//    c->merge(conf);
//    c->setDefault(conf);

//    c->iterateNodes([&](const std::string& key, auto&& var) {
//        TRACE_LOG(logject) << "Config: variable loaded: " << key;
//        variableUpdateSlot(key, var);
//    });

//    g_signal_connections.emplace_back(c->getVariableUpdatedSignal().connect(variableUpdateSlot));
//}

//extern "C" BOOST_SYMBOL_EXPORT void registerConfig(Config::Manager* confman) {
//    conf.putNode("frames", frames);
//    conf.putNode("resample", resample);
//    auto base = confman->getConfigRoot().synchronize();
//    loadedSlot(*base);
//}

//extern "C" BOOST_SYMBOL_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
//    *info = ::alsaInfo;
//    funs << registerOutput << registerConfig;
//}

} // namespace alsa
