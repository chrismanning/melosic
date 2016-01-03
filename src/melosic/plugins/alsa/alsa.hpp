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

#ifndef MELOSIC_ALSA_PLUGIN
#define MELOSIC_ALSA_PLUGIN

#include <melosic/melin/exports.hpp>
#include <melosic/melin/logging.hpp>

namespace alsa {
extern const Melosic::Plugin::Info alsaInfo;
extern Melosic::Logger::Logger logject;
} // namespace alsa

#define ALSA_THROW_IF(Exc, ret)                                                                                        \
    if(ret != 0) {                                                                                                     \
        using namespace Melosic;                                                                                       \
        BOOST_THROW_EXCEPTION(Exc() << ErrorTag::Output::DeviceErrStr(strdup(snd_strerror(ret)))                       \
                                    << ErrorTag::Plugin::Info(::alsa::alsaInfo));                                      \
    }

#endif // MELOSIC_ALSA_PLUGIN
