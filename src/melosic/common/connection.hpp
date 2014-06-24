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

#ifndef MELOSIC_CONNECTION_HPP
#define MELOSIC_CONNECTION_HPP

#include <memory>
#include <functional>
#include <atomic>

#include <melosic/common/signal_fwd.hpp>

namespace Melosic {
namespace Signals {

struct Connection;

struct ConnErasure {
    virtual ~ConnErasure() {}
    virtual void disconnect(Connection&) = 0;
    virtual bool isConnected(const Connection&) const noexcept = 0;
};

template <typename... Args>
struct ConnImpl : ConnErasure {
    explicit ConnImpl(std::weak_ptr<detail::SignalImpl<Args...>> sig) noexcept : sig(sig) {}

private:
    std::weak_ptr<detail::SignalImpl<Args...>> sig;

public:
    void disconnect(Connection& conn) override {
        if(auto ptr = sig.lock())
            ptr->disconnect(conn);
    }

    bool isConnected(const Connection& conn) const noexcept override {
        if(auto ptr = sig.lock())
            return ptr->connected(conn);
        return false;
    }
};

struct Connection {
    Connection() noexcept = default;

    bool operator==(const Connection& b) const noexcept {
        return pimpl == b.pimpl;
    }

private:
    std::shared_ptr<ConnErasure> pimpl;

    template <typename...>
    friend class detail::SignalImpl;

    template <typename ...Args>
    Connection(std::shared_ptr<detail::SignalImpl<Args...>> sig) :
        pimpl(std::make_shared<ConnImpl<Args...>>(sig)) {}

public:
    void disconnect() {
        if(auto nimpl = pimpl)
            nimpl->disconnect(*this);
    }

    bool isConnected() const noexcept {
        if(auto nimpl = pimpl)
            return nimpl->isConnected(*this);
        return false;
    }

    void swap(Connection& b) noexcept {
        using std::swap;
        swap(pimpl, b.pimpl);
    }
};

inline void swap(Connection& a, Connection& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

struct ScopedConnection : Connection {
    ScopedConnection() noexcept = default;
    ScopedConnection(ScopedConnection&& conn) noexcept = default;

    ScopedConnection& operator=(ScopedConnection&& conn) & {
        disconnect();
        swap(conn);
        return *this;
    }

    ScopedConnection(const Connection& conn) noexcept : Connection(conn) {}

    ScopedConnection(Connection&& conn) noexcept : Connection(std::move(conn)) {}
    ScopedConnection& operator=(Connection conn) & {
        return *this = ScopedConnection(std::move(conn));
    }

    ScopedConnection(const ScopedConnection&) noexcept = delete;
    ScopedConnection& operator=(const ScopedConnection&) noexcept = delete;

    ~ScopedConnection() noexcept(noexcept(disconnect())) {
        disconnect();
        assert(!isConnected());
    }
};

}
}

#endif // MELOSIC_CONNECTION_HPP
