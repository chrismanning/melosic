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
using boost::filesystem::absolute;

#include <melosic/melin/exports.hpp>
#include <melosic/common/range.hpp>

namespace Melosic {
class Kernel;

namespace Plugin {

class Manager {
public:
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;

    void loadPlugin(const boost::filesystem::path& filepath);
    void loadPlugins();

    void initialise();
    bool initialised() const;

    ForwardRange<const Info> getPlugins() const;

private:
    Manager(Kernel& kernel);
    friend class Kernel;
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // end namespace Plugin
} // end namespace Melosic

#endif // MELOSIC_PLUGIN_HPP
