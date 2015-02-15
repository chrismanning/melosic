/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#ifndef FLAC_MELIN_EXPORTS_HPP
#define FLAC_MELIN_EXPORTS_HPP

#include <boost/config.hpp>

#ifdef FLAC_MELIN_EXPORTS
#   define FLAC_MELIN_API BOOST_SYMBOL_EXPORT
#else
#   define FLAC_MELIN_API BOOST_SYMBOL_IMPORT
#endif

#ifndef _WIN32
#define FLAC_MELIN_LOCAL [[gnu::visibility("hidden")]]
#else
#define FLAC_MELIN_LOCAL
#endif

#endif // FLAC_MELIN_EXPORTS_HPP
