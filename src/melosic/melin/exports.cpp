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

#include <melosic/melin/kernel.hpp>

#include "exports.hpp"

namespace Melosic {

RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerInput_T& /*fun*/) {
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerDecoder_T& fun) {
    l.push_back(std::bind(fun, &k.getDecoderManager()));
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerOutput_T& fun) {
    l.push_back(std::bind(fun, &k.getOutputManager()));
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerEncoder_T& /*fun*/) {
    return *this;
}
RegisterFuncsInserter& RegisterFuncsInserter::operator<<(const registerConfig_T& fun) {
    l.push_back(std::bind(fun, &k.getConfigManager()));
    return *this;
}

}
