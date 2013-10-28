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

#include <boost/system/error_code.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/strand.hpp>

#include <melosic/common/audiospecs.hpp>
#include <melosic/melin/output.hpp>

namespace Melosic {
namespace ASIO {

using namespace boost::asio;

struct MELOSIC_EXPORT AudioOutputServiceBase {
    explicit AudioOutputServiceBase(io_service& service) :
        m_service(service),
        m_strand(m_service)
    {}

    virtual ~AudioOutputServiceBase() {}

    io_service::strand& get_strand() noexcept {
        return m_strand;
    }

    virtual void destroy() = 0;
    virtual void cancel(boost::system::error_code&) noexcept = 0;
    virtual void assign(Output::DeviceName dev_name, boost::system::error_code& ec) noexcept = 0;

    virtual AudioSpecs prepare(const AudioSpecs, boost::system::error_code&) noexcept = 0;
    virtual void play(boost::system::error_code&) noexcept = 0;
    virtual void pause(boost::system::error_code&) noexcept = 0;
    virtual void unpause(boost::system::error_code&) noexcept = 0;
    virtual void stop(boost::system::error_code&) noexcept = 0;
    virtual size_t write_some(const const_buffer&, boost::system::error_code&) noexcept = 0;

    typedef std::function<void(boost::system::error_code, AudioSpecs)> PrepareHandler;
    virtual void async_prepare(const AudioSpecs, PrepareHandler) noexcept = 0;

    typedef std::function<void(boost::system::error_code, std::size_t)> WriteHandler;
    virtual void async_write_some(const const_buffer&, WriteHandler) = 0;

    virtual bool non_blocking() const noexcept = 0;
    virtual void non_blocking(bool, boost::system::error_code&) noexcept = 0;

    virtual Output::DeviceState state() const noexcept = 0;
    virtual AudioSpecs current_specs() const noexcept = 0;

protected:
    io_service& get_io_service() noexcept {
        return m_service;
    }

private:
    io_service& m_service;
    io_service::strand m_strand;
};

}
}

#endif // MELOSIC_AUDIO_IMPL_HPP
