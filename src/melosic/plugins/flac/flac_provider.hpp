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

#ifndef FLAC_PROVIDER_HPP
#define FLAC_PROVIDER_HPP

#include <istream>
#include <memory>

#include <melosic/melin/decoder.hpp>

#include "./exports.hpp"

namespace flac {

struct FLAC_MELIN_API provider : public Melosic::Decoder::provider {
    provider() noexcept = default;

    virtual bool supports_mime(std::string_view mime_type) const override;
    virtual std::unique_ptr<Melosic::Decoder::PCMSource> make_decoder(std::unique_ptr<std::istream> in) const override;
    virtual bool verify(std::unique_ptr<std::istream> in) const override;
};

} // namespace flac

#endif // FLAC_PROVIDER_HPP