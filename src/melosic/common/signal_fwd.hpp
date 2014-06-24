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

#ifndef MELOSIC_SIGNALS_FWD_HPP
#define MELOSIC_SIGNALS_FWD_HPP

namespace Melosic {
namespace Signals {

template <typename Ret, typename ...Args>
struct Signal;
template <typename Ret, typename ...Args>
struct SignalCore;

namespace detail {
template <typename ...Args>
class SignalImpl;
}

struct use_future_t {};
constexpr use_future_t use_future = use_future_t{};

} // namespace Signals
} // namespace Melosic

#endif // MELOSIC_SIGNALS_FWD_HPP
