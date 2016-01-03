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

#include <boost/dll.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/decoder.hpp>

#include "./exports.hpp"
#include "wavpackdecoder.hpp"
#include "wavpack_provider.hpp"

namespace wavpack {

constexpr Plugin::Version current_version{1, 0, 0};
constexpr Plugin::Type type = Plugin::Type::decoder;

Plugin::Info wavpack_info{"Wavpack", type, current_version};

Plugin::Info plugin_info() {
    return wavpack_info;
}
MELOSIC_DLL_TYPED_ALIAS(plugin_info)

Decoder::provider* decoder_provider() {
    return new provider;
}
MELOSIC_DLL_TYPED_ALIAS(decoder_provider)

} // namespace wavpack
