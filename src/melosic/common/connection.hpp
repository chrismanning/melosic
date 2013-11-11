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
    virtual bool isConnected() const noexcept = 0;
};

template <typename SigType>
struct ConnImpl : ConnErasure {
    explicit ConnImpl(SigType& sig) noexcept : sig(sig), connected(true) {}

private:
    SigType& sig;
    std::atomic_bool connected;

public:
    void disconnect(Connection& conn) noexcept(noexcept(sig.disconnect(conn))) override {
        if(connected.load())
            connected.exchange(!sig.disconnect(conn));
    }

    bool isConnected() const noexcept override {
        return connected.load();
    }
};

struct ConnHash;

struct Connection {
    Connection() noexcept = default;
    Connection(const Connection&) noexcept = default;
    Connection& operator=(const Connection&) noexcept = default;
    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;

    bool operator==(const Connection& b) const noexcept {
        return pimpl == b.pimpl;
    }

private:
    std::shared_ptr<ConnErasure> pimpl;

public:
    template <typename R, typename ...Args>
    Connection(SignalCore<R(Args...)>& sig) :
        pimpl(new ConnImpl<SignalCore<R(Args...)>>(sig)) {}

    void disconnect() noexcept(noexcept(pimpl->disconnect(*this))) {
        if(pimpl)
            pimpl->disconnect(*this);
    }
    bool isConnected() const noexcept(noexcept(pimpl.operator ->())) {
        if(pimpl)
            return pimpl->isConnected();
        return false;
    }

    void swap(Connection& b) noexcept {
        using std::swap;
        swap(pimpl, b.pimpl);
    }

private:
    friend struct ConnHash;
};

inline void swap(Connection& a, Connection& b) noexcept {
    a.swap(b);
}

struct ConnHash {
    ConnHash() noexcept = default;
    size_t operator()(const Connection& conn) const noexcept {
        return std::hash<std::shared_ptr<ConnErasure>>()(conn.pimpl);
    }
};

struct ScopedConnection : Connection {
    ScopedConnection() noexcept = default;
    ScopedConnection(ScopedConnection&& conn) noexcept(noexcept(disconnect())) : Connection((disconnect(), conn)) {}

    ScopedConnection& operator=(ScopedConnection&& conn)
    noexcept(std::is_nothrow_move_constructible<ScopedConnection>::value)
    {
        using std::swap;
        swap(conn, *this);
        return *this;
    }

    ScopedConnection(const Connection& conn) noexcept : Connection(conn) {}

    ScopedConnection(Connection&& conn) noexcept : Connection(conn) {}
    ScopedConnection& operator=(Connection conn) noexcept {
        return *this = ScopedConnection(conn);
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
