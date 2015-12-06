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

#include <memory>

#include <wavpack/wavpack.h>

#include <boost/config.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/decoder.hpp>

#include "./exports.hpp"
#include "./wavpackdecoder.hpp"
#include "wavpack_provider.hpp"

constexpr Plugin::Version current_version{1, 0, 0};
constexpr Plugin::Type type = Plugin::Type::decoder;

const Plugin::Info wavpack_info{"Wavpack", type, current_version};

extern "C" BOOST_SYMBOL_EXPORT wavpack::provider* decoder_provider() {
    return new wavpack::provider;
}

extern "C" BOOST_SYMBOL_EXPORT const Plugin::Info* plugin_info() {
    return &::wavpack_info;
}
