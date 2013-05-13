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

#ifndef MELOSIC_SLOTS_HPP
#define MELOSIC_SLOTS_HPP

#include <memory>

namespace Melosic {
namespace Thread {
class Manager;
}
namespace Slots {

class Manager {
public:
    Manager();
    ~Manager();

    template <typename T>
    T& get();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

} // namespace Slots
} // namespace Melosic

#endif // MELOSIC_SLOTS_HPP
