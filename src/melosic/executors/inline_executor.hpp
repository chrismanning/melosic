/**************************************************************************
**  Copyright (C) 2014 Christian Manning
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

#ifndef MELOSIC_EXECUTORS_INLINE_EXECUTOR_HPP
#define MELOSIC_EXECUTORS_INLINE_EXECUTOR_HPP

namespace Melosic {
namespace executors {

struct inline_executor {
    inline_executor() = default;

    template <typename WorkT>
    inline void submit(WorkT&& work) {
        work();
    }
};

} // namespace executors
} // namespace Melosic

#endif // MELOSIC_EXECUTORS_INLINE_EXECUTOR_HPP