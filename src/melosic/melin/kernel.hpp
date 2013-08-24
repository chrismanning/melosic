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

#ifndef MELOSIC_CORE_KERNEL_HPP
#define MELOSIC_CORE_KERNEL_HPP

#include <memory>

#include <melosic/common/common.hpp>

namespace Melosic {

namespace Plugin {
class Manager;
}
namespace Input {
class Manager;
}
namespace Decoder {
class Manager;
}
namespace Output {
class Manager;
}
namespace Encoder {
class Manager;
}
namespace Config {
class Manager;
}
namespace Thread {
class Manager;
}
namespace Playlist {
class Manager;
}

namespace Core {

class Kernel {
public:
    Kernel();

    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;
    Kernel(Kernel&&) = delete;
    Kernel& operator=(Kernel&&) = delete;

    ~Kernel();

    MELOSIC_EXPORT Config::Manager& getConfigManager();
    MELOSIC_EXPORT Plugin::Manager& getPluginManager();
    MELOSIC_EXPORT Input::Manager& getInputManager();
    MELOSIC_EXPORT Decoder::Manager& getDecoderManager();
    MELOSIC_EXPORT Output::Manager& getOutputManager();
    MELOSIC_EXPORT Encoder::Manager& getEncoderManager();
    MELOSIC_EXPORT Thread::Manager& getThreadManager();
    MELOSIC_EXPORT Playlist::Manager& getPlaylistManager();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_CORE_KERNEL_HPP
