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

#ifndef MELOSIC_EXPORTS_HPP
#define MELOSIC_EXPORTS_HPP

#ifdef WIN32
#ifdef MELOSIC_PLUGIN_EXPORTS
#define MELOSIC_EXPORT __declspec(dllexport)
#else
#define MELOSIC_EXPORT __declspec(dllimport)
#endif
#else
#define MELOSIC_EXPORT
#endif

namespace Melosic {
class IKernel;
}

#include <functional>

extern "C" void registerPluginObjects(Melosic::IKernel& k);
typedef decltype(registerPluginObjects) registerPlugin_F;
typedef std::function<registerPlugin_F> registerPlugin_T;
extern "C" void destroyPluginObjects();
typedef decltype(destroyPluginObjects) destroyPlugin_F;
typedef std::function<destroyPlugin_F> destroyPlugin_T;
extern "C" int startEventLoop(int argc, char ** argv, Melosic::IKernel& k);

#endif // MELOSIC_EXPORTS_HPP
