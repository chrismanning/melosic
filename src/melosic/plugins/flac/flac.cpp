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

#include <boost/dll/alias.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/decoder.hpp>
using namespace Melosic;

#include "flacdecoder.hpp"
#include "flac_provider.hpp"

namespace flac {

Plugin::Info flacInfo{"FLAC", Plugin::Type::decoder, {1, 0, 0}};

Plugin::Info plugin_info() {
    return flacInfo;
}
BOOST_DLL_AUTO_ALIAS(plugin_info)

Decoder::provider* decoder_provider() {
    return new provider;
}
BOOST_DLL_AUTO_ALIAS(decoder_provider)

} // namespace flac
