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

#ifndef MELOSIC_PLUGIN_HPP
#define MELOSIC_PLUGIN_HPP

#include <memory>

#include <boost/filesystem.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/common/range.hpp>
#include <melosic/common/signal_fwd.hpp>

namespace Melosic {

namespace Core {
class Kernel;
}

namespace Signals {
namespace Plugin {
using PluginsLoaded = SignalCore<void(ForwardRange<const Melosic::Plugin::Info>)>;
}
}

namespace Plugin {

class Manager final {
  public:
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;

    MELOSIC_EXPORT void loadPlugins(Core::Kernel& kernel);

    std::vector<Info> getPlugins() const;
    Signals::Plugin::PluginsLoaded& getPluginsLoadedSignal() const;

  private:
    explicit Manager(const std::shared_ptr<Config::Manager>& confman);
    friend class Core::Kernel;
    struct impl;
    std::unique_ptr<impl> pimpl;
};

} // end namespace Plugin
} // end namespace Melosic

#endif // MELOSIC_PLUGIN_HPP
