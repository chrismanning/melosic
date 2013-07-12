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
    virtual bool isConnected() = 0;
};

template <typename SigType>
struct ConnImpl : ConnErasure {
    ConnImpl(SigType& sig) : sig(sig), connected(true) {}

    void disconnect(Connection& conn) override {
        if(connected)
            connected = !sig.disconnect(conn);
    }

    bool isConnected() override {
        return connected;
    }

private:
    SigType& sig;
    std::atomic_bool connected;
};

struct ConnHash;

struct Connection {
    Connection() = default;
    Connection(const Connection&) = default;
    Connection& operator=(const Connection&) = default;
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    bool operator==(const Connection& b) const {
        return pimpl == b.pimpl;
    }

    template <typename R, typename ...Args>
    Connection(SignalCore<R(Args...)>& sig)
        : pimpl(new ConnImpl<SignalCore<R(Args...)>>(sig)) {}

    void disconnect() {
        if(pimpl)
            pimpl->disconnect(*this);
    }
    bool isConnected() const {
        if(pimpl)
            return pimpl->isConnected();
        return false;
    }

private:
    friend struct ConnHash;
    std::shared_ptr<ConnErasure> pimpl;
};

struct ConnHash {
    size_t operator()(const Connection& conn) const {
        return std::hash<std::shared_ptr<ConnErasure>>()(conn.pimpl);
    }
};

struct ScopedConnection : Connection {
    ScopedConnection() = default;
    ScopedConnection(ScopedConnection&& conn) = default;
    ScopedConnection& operator=(ScopedConnection&& conn) = default;

    ScopedConnection(const Connection& conn) : Connection(conn) {}

    ScopedConnection(Connection&& conn) : Connection(conn) {}
    ScopedConnection& operator=(Connection&& conn) {
        return *this = ScopedConnection(conn);
    }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ~ScopedConnection()  {
        disconnect();
        assert(!isConnected());
    }
};

}
}

#endif // MELOSIC_CONNECTION_HPP
