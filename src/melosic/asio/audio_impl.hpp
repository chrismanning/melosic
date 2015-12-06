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

#ifndef MELOSIC_AUDIO_IMPL_HPP
#define MELOSIC_AUDIO_IMPL_HPP

#include <memory>
#include <system_error>

#include <asio/buffer.hpp>
#include <asio/strand.hpp>

#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/output.hpp>

namespace Melosic {
namespace AudioIO {

struct MELOSIC_EXPORT AudioOutputServiceBase {
    explicit AudioOutputServiceBase(asio::io_service& service) : m_service(service) {
    }

    virtual ~AudioOutputServiceBase() {
    }

    virtual void destroy() = 0;
    virtual void cancel(std::error_code&) = 0;
    virtual void assign(Output::device_descriptor dev_name, std::error_code& ec) = 0;

    virtual AudioSpecs prepare(const AudioSpecs, std::error_code&) = 0;
    virtual void play(std::error_code&) = 0;
    virtual void pause(std::error_code&) = 0;
    virtual void unpause(std::error_code&) = 0;
    virtual void stop(std::error_code&) = 0;
    virtual size_t write_some(const asio::const_buffer&, std::error_code&) = 0;

    typedef std::function<void(std::error_code, AudioSpecs)> PrepareHandler;
    virtual void async_prepare(AudioSpecs, PrepareHandler) = 0;

    typedef std::function<void(std::error_code, std::size_t)> WriteHandler;
    virtual void async_write_some(const asio::const_buffer&, WriteHandler) = 0;

    virtual bool non_blocking() const = 0;
    virtual void non_blocking(bool, std::error_code&) = 0;

    virtual Output::DeviceState state() const = 0;
    virtual AudioSpecs current_specs() const = 0;

  protected:
    asio::io_service& get_io_service() noexcept {
        return m_service;
    }

  private:
    asio::io_service& m_service;
};
}
}

#endif // MELOSIC_AUDIO_IMPL_HPP
