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

#ifndef MELOSIC_INPUTMANAGER_HPP
#define MELOSIC_INPUTMANAGER_HPP

#include <memory>
#include <istream>

#include <cpprest/uri.h>

#include <melosic/common/common.hpp>

namespace Melosic {
struct AudioSpecs;
namespace Input {

class MELOSIC_EXPORT Manager {
  public:
    Manager();
    ~Manager();

    Manager(Manager&&) = delete;
    Manager(const Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;

    MELOSIC_EXPORT std::unique_ptr<std::istream> open(const web::uri& uri) const;

  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

MELOSIC_EXPORT boost::filesystem::path uri_to_path(const web::uri& uri);

MELOSIC_EXPORT web::uri to_uri(const boost::filesystem::path& path);

} // namespace Input
} // namespace Melosic

#endif // MELOSIC_INPUTMANAGER_HPP
