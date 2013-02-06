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

#include <string>
#include <functional>
#include <map>

#include <boost/filesystem.hpp>
using boost::filesystem::path;

#include <melosic/common/error.hpp>
#include <melosic/common/file.hpp>
#include <melosic/core/kernel.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/core/track.hpp>
#include <melosic/core/taglibfile.hpp>
#include <melosic/core/player.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/encoder.hpp>

namespace Melosic {

static Logger::Logger logject(boost::log::keywords::channel = "Kernel");

class Kernel::impl {
    impl(Kernel& k) : plugman(k) {}
    Plugin::Manager plugman;
    Player player;
    Input::Manager inman;
    Decoder::Manager decman;
    Output::Manager outman;
    Encoder::Manager encman;
    Config::Manager config;
    friend class Kernel;
};

Kernel::Kernel() : pimpl(new impl(*this)) {}

Kernel::~Kernel() {}

Player& Kernel::getPlayer() {
    return pimpl->player;
}

Config::Manager& Kernel::getConfigManager() {
    return pimpl->config;
}

Input::Manager& Kernel::getInputManager() {
    return pimpl->inman;
}

Decoder::Manager& Kernel::getDecoderManager() {
    return pimpl->decman;
}

Output::Manager& Kernel::getOutputManager() {
    return pimpl->outman;
}

Encoder::Manager& Kernel::getEncoderManager() {
    return pimpl->encman;
}

Plugin::Manager& Kernel::getPluginManager() {
    return pimpl->plugman;
}

} // end namespace Melosic

