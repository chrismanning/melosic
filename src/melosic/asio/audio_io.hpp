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

#ifndef MELOSIC_AUDIO_IO_HPP
#define MELOSIC_AUDIO_IO_HPP

#include <asio/buffer.hpp>

#include <melosic/asio/audio_service.hpp>

namespace Melosic {
namespace AudioIO {

struct MELOSIC_EXPORT AudioOutputBase {
    virtual ~AudioOutputBase() {
    }

    void cancel(std::error_code& ec) {
        get_service().cancel(get_implementation(), ec);
    }
    void cancel() {
        std::error_code ec;
        cancel(ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    void assign(Output::DeviceName dev_name, std::error_code& ec) {
        get_service().assign(get_implementation(), std::move(dev_name), ec);
    }
    void assign(Output::DeviceName dev_name) {
        std::error_code ec;
        assign(std::move(dev_name), ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    void prepare(const AudioSpecs as, std::error_code& ec) {
        get_service().prepare(get_implementation(), as, ec);
    }
    void prepare(const AudioSpecs as) {
        std::error_code ec;
        prepare(as, ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    void play(std::error_code& ec) {
        get_service().play(get_implementation(), ec);
    }
    void play() {
        std::error_code ec;
        play(ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    void pause(std::error_code& ec) {
        get_service().pause(get_implementation(), ec);
    }
    void pause() {
        std::error_code ec;
        pause(ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    void unpause(std::error_code& ec) {
        get_service().unpause(get_implementation(), ec);
    }
    void unpause() {
        std::error_code ec;
        unpause(ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    void stop(std::error_code& ec) {
        get_service().stop(get_implementation(), ec);
    }
    void stop() {
        std::error_code ec;
        stop(ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    template <typename ConstBufferSequence> size_t write_some(const ConstBufferSequence& buffers, std::error_code& ec) {
        return get_service().write_some(get_implementation(), buffers, ec);
    }

    template <typename ConstBufferSequence> size_t write_some(const ConstBufferSequence& buffers) {
        std::error_code ec;
        auto r = write_some(buffers, ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
        return r;
    }

    AudioSpecs current_specs() const {
        return get_service().current_specs(get_implementation());
    }

    Output::DeviceState state() const {
        return get_service().state(get_implementation());
    }

    bool non_blocking() const {
        return get_service().non_blocking(get_implementation());
    }

    void non_blocking(bool mode, std::error_code& ec) {
        get_service().non_blocking(get_implementation(), mode, ec);
    }
    void non_blocking(bool mode) {
        std::error_code ec;
        non_blocking(mode, ec);
        if(ec)
            BOOST_THROW_EXCEPTION(std::system_error(ec));
    }

    template <typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(const ConstBufferSequence& buf, WriteHandler&& handler) {
        get_service().async_write_some(get_implementation(), buf, std::forward<WriteHandler>(handler));
    }

    typedef AudioOutputService<AudioOutputServiceBase> service_type;
    typedef service_type::implementation_type implementation_type;

    implementation_type& get_implementation() noexcept {
        return implementation;
    }
    const implementation_type& get_implementation() const noexcept {
        return implementation;
    }

  protected:
    AudioOutputBase() = default;

    virtual service_type& get_service() = 0;
    virtual const service_type& get_service() const = 0;

  private:
    implementation_type implementation;
};

template <typename...> struct BasicAudioOutput;

template <typename ServiceImpl> struct BasicAudioOutput<AudioOutputService<ServiceImpl>> : AudioOutputBase {
    using Service = AudioOutputService<ServiceImpl>;

    explicit BasicAudioOutput(asio::io_service& service)
        : AudioOutputBase(), m_service(asio::use_service<Service>(service)) {
        m_service.construct(get_implementation());
    }

    BasicAudioOutput(asio::io_service& service, Output::DeviceName dev_name) : BasicAudioOutput(service) {
        std::error_code ec;
        get_service().assign(get_implementation(), std::move(dev_name), ec);
    }

    BasicAudioOutput(BasicAudioOutput&&) = delete;

    ~BasicAudioOutput() {
        m_service.destroy(get_implementation());
    }

    service_type& get_service() override {
        return reinterpret_cast<service_type&>(m_service);
    }

    const service_type& get_service() const override {
        return reinterpret_cast<service_type&>(m_service);
    }

  private:
    Service& m_service;
};
}
}

#endif // MELOSIC_AUDIO_IO_HPP
