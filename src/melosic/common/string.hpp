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

#ifndef MELOSIC_STRING_HPP
#define MELOSIC_STRING_HPP

#include <boost/locale/conversion.hpp>

#define toUpper(str) boost::locale::to_upper(str)
#define toLower(str) boost::locale::to_lower(str)
#define toTitle(str) boost::locale::to_title(str)

#endif // MELOSIC_STRING_HPP
