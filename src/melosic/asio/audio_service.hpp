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

#ifndef MELOSIC_AUDIO_SERVICE_HPP
#define MELOSIC_AUDIO_SERVICE_HPP

#include <asio/io_service.hpp>

#include <melosic/asio/audio_impl.hpp>

namespace Melosic {
namespace AudioIO {

template <typename AudioServiceImpl>
struct MELOSIC_EXPORT AudioOutputService : asio::io_service::service {
    static_assert(std::is_base_of<AudioOutputServiceBase, AudioServiceImpl>::value, "");

    static asio::io_service::id id;

    typedef std::shared_ptr<AudioOutputServiceBase> implementation_type;

    explicit AudioOutputService(asio::io_service& io_service) :
        asio::io_service::service(io_service)
    {}

    void construct(implementation_type& impl) {
        impl.reset(new AudioServiceImpl(get_io_service()));
    }

    void destroy(implementation_type& impl) {
        impl->destroy();
        impl.reset();
    }

    void cancel(implementation_type& impl, std::error_code& ec) {
        impl->cancel(ec);
    }

    void assign(implementation_type& impl, Output::DeviceName dev_name, std::error_code& ec) {
        impl->assign(std::move(dev_name), ec);
    }

    void prepare(implementation_type& impl, const AudioSpecs as, std::error_code& ec) {
        impl->prepare(as, ec);
    }

    void play(implementation_type& impl, std::error_code& ec) {
        impl->play(ec);
    }

    void pause(implementation_type& impl, std::error_code& ec) {
        impl->pause(ec);
    }

    void unpause(implementation_type& impl, std::error_code& ec) {
        impl->unpause(ec);
    }

    void stop(implementation_type& impl, std::error_code& ec) {
        impl->stop(ec);
    }

    template <typename ConstBufferSequence>
    size_t write_some(implementation_type& impl, const ConstBufferSequence& buf, std::error_code& ec) {
        if(buf.begin() == buf.end()) {
            ec = asio::error::make_error_code(asio::error::no_buffer_space);
            return 0;
        }
        return impl->write_some(*buf.begin(), ec);
    }
    size_t write_some(implementation_type& impl, const asio::const_buffer& buf, std::error_code& ec) {
        return impl->write_some(buf, ec);
    }

    Output::DeviceState state(const implementation_type& impl) const {
        return impl->state();
    }

    AudioSpecs current_specs(const implementation_type& impl) const {
        return impl->current_specs();
    }

    bool non_blocking(const implementation_type& impl) const {
        return impl->non_blocking();
    }
    void non_blocking(const implementation_type& impl, bool mode, std::error_code& ec) {
        impl->non_blocking(mode, ec);
    }

    template <typename PrepareHandler>
    void async_prepare(implementation_type& impl, const AudioSpecs, PrepareHandler handler);

    template <typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(implementation_type& impl, const ConstBufferSequence& buf, WriteHandler&& handler) {
        impl->async_write_some(buf, std::forward<WriteHandler>(handler));
    }

private:
    void shutdown_service() override {}
};
template <typename AudioServiceImpl>
asio::io_service::id AudioOutputService<AudioServiceImpl>::id;

}
}

#endif // MELOSIC_AUDIO_SERVICE_HPP
