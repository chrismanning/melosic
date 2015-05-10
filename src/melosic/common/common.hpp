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

#ifndef MELOSIC_COMMON_HPP
#define MELOSIC_COMMON_HPP

#include <cstdint>
#include <chrono>
namespace chrono = std::chrono;
#include <string>
#include <experimental/string_view>

#include <boost/config.hpp>

namespace std {
namespace experimental {}
using namespace experimental;
}

namespace Melosic {
using namespace std::literals;
}

#ifdef MELOSIC_EXPORTS
#   define MELOSIC_EXPORT BOOST_SYMBOL_EXPORT
#else
#   define MELOSIC_EXPORT BOOST_SYMBOL_IMPORT
#endif

#ifndef _WIN32
#define MELOSIC_LOCAL [[gnu::visibility("hidden")]]
#else
#define MELOSIC_LOCAL
#endif

#endif // MELOSIC_COMMON_HPP
