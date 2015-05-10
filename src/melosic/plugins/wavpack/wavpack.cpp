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

#include <wavpack/wavpack.h>

#include <deque>
#include <algorithm>
#include <cmath>
#include <fstream>

#include <boost/config.hpp>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/decoder.hpp>
using namespace Melosic;

#include "./exports.hpp"
#include "./wavpackdecoder.hpp"

constexpr Plugin::Info wavpackInfo{"Wavpack", Plugin::Type::decode, {1, 0, 0}};

extern "C" BOOST_SYMBOL_EXPORT void registerPlugin(Plugin::Info* info, RegisterFuncsInserter funs) {
    *info = ::wavpackInfo;
    funs << registerDecoder;
}

extern "C" BOOST_SYMBOL_EXPORT void registerDecoder(Decoder::Manager* decman) {
    decman->addAudioFormat([](auto input) { return std::make_unique<WavpackDecoder>(std::move(input)); },
                           std::string_view("audio/x-wavpack"));
}

extern "C" BOOST_SYMBOL_EXPORT void destroyPlugin() {}
