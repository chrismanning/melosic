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

namespace asio {
class io_service;
}

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
namespace Playlist {
class Manager;
}
namespace Library {
class Manager;
}

namespace Core {

class MELOSIC_EXPORT Kernel final {
  public:
    Kernel();

    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;
    Kernel(Kernel&&) = delete;
    Kernel& operator=(Kernel&&) = delete;

    ~Kernel();

    std::shared_ptr<Config::Manager> getConfigManager() const;
    std::shared_ptr<Plugin::Manager> getPluginManager() const;
    std::shared_ptr<Input::Manager> getInputManager() const;
    std::shared_ptr<Decoder::Manager> getDecoderManager() const;
    std::shared_ptr<Output::Manager> getOutputManager() const;
    std::shared_ptr<Encoder::Manager> getEncoderManager() const;
    std::shared_ptr<Melosic::Playlist::Manager> getPlaylistManager() const;
    std::shared_ptr<Library::Manager> getLibraryManager() const;

    asio::io_service& getIOService();

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Core
} // namespace Melosic

#endif // MELOSIC_CORE_KERNEL_HPP
