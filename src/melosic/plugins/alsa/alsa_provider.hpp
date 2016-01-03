/**************************************************************************
**  Copyright (C) 2016 Christian Manning
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

#ifndef ALSA_PROVIDER_HPP
#define ALSA_PROVIDER_HPP

#include <vector>
#include <memory>

#include <melosic/melin/output.hpp>

namespace alsa {

struct provider : Melosic::Output::provider {
    provider() noexcept = default;

    virtual std::vector<Melosic::Output::device_descriptor> available_devices() const override;
    virtual std::string device_prefix() const override;

    virtual std::unique_ptr<Melosic::AudioIO::AudioOutputBase>
    make_output(asio::io_service&, const Melosic::Output::device_descriptor&) const override;
};

} // namespace alsa

#endif // ALSA_PROVIDER_HPP
