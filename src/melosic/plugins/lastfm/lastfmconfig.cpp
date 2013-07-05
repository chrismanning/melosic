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

#include "lastfmconfig.hpp"

#include <melosic/common/string.hpp>
#include <melosic/melin/logging.hpp>

namespace Melosic {

static Logger::Logger logject(logging::keywords::channel = "LastFMConfig");

LastFmConfig::LastFmConfig() :
    Config::Config("LastFM")
{}

LastFmConfig::~LastFmConfig() {}

} //end namespace Melosic

BOOST_CLASS_EXPORT_IMPLEMENT(Melosic::LastFmConfig)
