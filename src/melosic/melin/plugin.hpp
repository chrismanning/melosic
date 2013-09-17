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
#include <condition_variable>

#include <boost/filesystem.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/common/range.hpp>

namespace Melosic {

namespace Core {
class Kernel;
}

namespace Plugin {

class Manager {
public:
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;

    void loadPlugins(Core::Kernel& kernel);

    MELOSIC_EXPORT ForwardRange<const Info> getPlugins() const;

private:
    explicit Manager(Config::Manager& confman);
    friend class Core::Kernel;
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // end namespace Plugin
} // end namespace Melosic

#endif // MELOSIC_PLUGIN_HPP
