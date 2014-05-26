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

#ifndef MELOSIC_CONFIG_VARIABLE_UPDATED_HPP
#define MELOSIC_CONFIG_VARIABLE_UPDATED_HPP

#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/configvar.hpp>

namespace Melosic {

namespace Config {
class Conf;
}

namespace Signals {

namespace Config {
typedef SignalCore<void(const std::string&, const Melosic::Config::VarType&)> VariableUpdated;
typedef SignalCore<
    void(std::reference_wrapper<boost::unique_lock_ptr<Melosic::Config::Conf, std::recursive_timed_mutex>>)> Loaded;
}
}
}

#endif // MELOSIC_CONFIG_VARIABLE_UPDATED_HPP
