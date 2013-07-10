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

#ifndef MELOSIC_SIGNALS_HPP
#define MELOSIC_SIGNALS_HPP

#include <memory>
#include <type_traits>

#include <boost/utility/result_of.hpp>

#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/signal_core.hpp>

namespace Melosic {
namespace Signals {

template <typename Ret, typename ...Args>
struct Signal<Ret (Args...)> : SignalCore<Ret (Args...)> {
    using SignalCore<Ret (Args...)>::SignalCore;

    template <typename ...A>
    void operator()(A&& ...args) {
        SignalCore<Ret (Args...)>::call(std::forward<A>(args)...);
    }

protected:
    typedef Signal<Ret (Args...)> Super;
};

template <typename Ret, typename ...Args>
struct Signal<SignalCore<Ret (Args...)>> : Signal<Ret (Args...)> {};

} // namespace Signals
} // namespace Melosic

#endif // MELOSIC_SIGNALS_HPP
