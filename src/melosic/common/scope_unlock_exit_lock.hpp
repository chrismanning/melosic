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

#ifndef MELOSIC_SCOPE_UNLOCK_EXIT_LOCK_HPP
#define MELOSIC_SCOPE_UNLOCK_EXIT_LOCK_HPP

namespace Melosic {
template <typename Lockable>
struct scope_unlock_exit_lock {
    explicit scope_unlock_exit_lock(Lockable& l) : l(l) { l.unlock(); }
    ~scope_unlock_exit_lock() { l.lock(); }
    Lockable& l;
};
}

#endif // MELOSIC_SCOPE_UNLOCK_EXIT_LOCK_HPP
