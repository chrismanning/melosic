/**************************************************************************
**  Copyright (C) 2013 Christian Manning
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

#ifndef MELOSIC_LIBRARY_HPP
#define MELOSIC_LIBRARY_HPP

#include <memory>

#include <boost/filesystem/path.hpp>

namespace Melosic {

namespace Config {
class Manager;
}

namespace Library {

class Manager final {
public:
    Manager(Config::Manager&);

    ~Manager();

    void addDirectory(boost::filesystem::path);
    void rescan();

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Library
} // namespace Melosic

#endif // MELOSIC_LIBRARY_HPP
