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

#ifndef ALSA_OUTPUT_SERVICE_IMPL_HPP
#define ALSA_OUTPUT_SERVICE_IMPL_HPP

#include <asio/posix/stream_descriptor.hpp>

#include <alsa/asoundlib.h>

#include <melosic/melin/exports.hpp>
#include <melosic/melin/output.hpp>
#include <melosic/asio/audio_impl.hpp>

#include "alsa.hpp"

namespace alsa {

struct MELOSIC_EXPORT output_service_impl : Melosic::AudioIO::AudioOutputServiceBase {
    using base_t = Melosic::AudioIO::AudioOutputServiceBase;

    explicit output_service_impl(asio::io_service& service);

    void assign(Melosic::Output::device_descriptor dev_name, std::error_code& ec) override;

    void destroy() override;

    void cancel(std::error_code& ec) override;

    Melosic::AudioSpecs prepare(Melosic::AudioSpecs as, std::error_code& ec) override;

    void play(std::error_code& ec) override;

    void pause(std::error_code& ec) override;

    void unpause(std::error_code& ec) override;

    void stop(std::error_code& ec) override;

    size_t write_some(const asio::const_buffer& buf, std::error_code& ec) override;

    void async_prepare(const Melosic::AudioSpecs, PrepareHandler) override;

    void async_write_some(const asio::const_buffer& buf, WriteHandler handler) override;

    Melosic::AudioSpecs current_specs() const override;

    Melosic::Output::DeviceState state() const override;

    bool non_blocking() const override;

    void non_blocking(bool mode, std::error_code& ec) override;

  private:
    snd_pcm_t* m_pdh = nullptr;
    snd_pcm_hw_params_t* m_params = nullptr;
    std::vector<pollfd> m_pfds;
    asio::posix::stream_descriptor m_asio_fd;
    bool m_non_blocking = true;
    bool m_mangled_revents = true;
    Melosic::AudioSpecs m_current_specs;
    std::string m_name{"default"};
    std::string m_desc;
    Melosic::Output::DeviceState m_state;
};

} // namespace alsa

#endif // ALSA_OUTPUT_SERVICE_IMPL_HPP
