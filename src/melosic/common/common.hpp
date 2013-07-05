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

#ifdef _WIN32
#   ifdef MELOSIC_CORE_EXPORTS
#       define MELOSIC_CORE_EXPORT __declspec(dllexport)
#   else
#       define MELOSIC_CORE_EXPORT __declspec(dllimport)
#   endif
#   ifdef MELOSIC_PLUGIN_EXPORTS
#       define MELOSIC_PLUGIN_EXPORT __declspec(dllexport)
#   else
#       define MELOSIC_PLUGIN_EXPORT __declspec(dllimport)
#   endif
#   ifdef MELOSIC_MELIN_EXPORTS
#       define MELOSIC_MELIN_EXPORT __declspec(dllexport)
#   else
#       define MELOSIC_MELIN_EXPORT __declspec(dllimport)
#   endif
#elif __GNUC__ >= 4
#   ifdef MELOSIC_CORE_EXPORTS
#       define MELOSIC_CORE_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define MELOSIC_CORE_EXPORT
#   endif
#   ifdef MELOSIC_PLUGIN_EXPORTS
#       define MELOSIC_PLUGIN_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define MELOSIC_PLUGIN_EXPORT
#   endif
#   ifdef MELOSIC_MELIN_EXPORTS
#       define MELOSIC_MELIN_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define MELOSIC_MELIN_EXPORT
#   endif
#else
#   define MELOSIC_CORE_EXPORT
#   define MELOSIC_MELIN_EXPORT
#   define MELOSIC_PLUGIN_EXPORT
#endif

#endif // MELOSIC_COMMON_HPP
