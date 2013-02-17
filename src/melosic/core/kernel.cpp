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

#include <melosic/core/player.hpp>
#include <melosic/melin/config.hpp>
#include <melosic/melin/plugin.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/melin/decoder.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/melin/encoder.hpp>
#include <melosic/melin/sigslots/slots.hpp>
#include <melosic/melin/thread.hpp>

#include "kernel.hpp"

namespace Melosic {

class Kernel::impl {
    impl(Kernel& k)
        : plugman(k),
          slotman(tman),
          confman(slotman),
          outman(confman, slotman, plugman),
          player(slotman, outman)
    {}

    Plugin::Manager plugman;
    Thread::Manager tman;
    Slots::Manager slotman;
    Config::Manager confman;
    Input::Manager inman;
    Decoder::Manager decman;
    Output::Manager outman;
    Encoder::Manager encman;
    Player player;
    friend class Kernel;
};

Kernel::Kernel() : pimpl(new impl(*this)) {}

Kernel::~Kernel() {}

Player& Kernel::getPlayer() {
    return pimpl->player;
}

Config::Manager& Kernel::getConfigManager() {
    return pimpl->confman;
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

Slots::Manager& Kernel::getSlotManager() {
    return pimpl->slotman;
}

Thread::Manager& Kernel::getThreadManager() {
    return pimpl->tman;
}

} // end namespace Melosic

