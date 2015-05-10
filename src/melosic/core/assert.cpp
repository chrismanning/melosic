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

#include <boost/config.hpp>
#include <boost/format.hpp>

#include <melosic/melin/logging.hpp>

Melosic::Logger::Logger logject{};

namespace boost {
BOOST_SYMBOL_EXPORT void assertion_failed(char const* expr, char const* function, char const* file, long line) {
    LOG(logject) << (boost::format("%1%:%2%: %3%: Assertion `%4%' failed.") % file % line % function % expr);
    std::abort();
}
BOOST_SYMBOL_EXPORT void assertion_failed_msg(char const* expr, char const* msg, char const* function, char const* file,
                                              long line) {
    LOG(logject) << (boost::format("%1%:%2%: %3%: Assertion `%4%' failed.\n") % file % line % function % expr) << msg;
    std::abort();
}
}
