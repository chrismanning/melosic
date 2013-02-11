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

#include <thread>
using std::mutex; using std::unique_lock; using std::lock_guard;

#include <melosic/melin/sigslots/signals_fwd.hpp>

namespace Melosic {
namespace Signals {

struct Connection;

namespace {
struct ConnErasure {
    virtual ~ConnErasure() {}
    virtual void disconnect(const Connection&) = 0;
    virtual bool isConnected() = 0;
};

template <typename SigType>
struct ConnImpl : ConnErasure {
    ConnImpl(SigType& sig, std::shared_ptr<void> ka)
        : sig(sig), ka(ka)
    {}

    void disconnect(const Connection& conn) {
        unique_lock<Mutex> l(mu);
        if(connected) {
            connected = false;
            l.unlock();
            sig.disconnect(conn);
        }
    }

    bool isConnected() {
        lock_guard<Mutex> l(mu);
        return connected;
    }

private:
    bool connected = true;
    SigType& sig;
    std::shared_ptr<void> ka;
    typedef mutex Mutex;
    Mutex mu;
};
}

struct ConnHash;

struct Connection {
    Connection() = default;
    Connection(const Connection&) = default;
    Connection& operator=(const Connection&) = default;
    Connection(Connection&& conn) = default;
    Connection& operator=(Connection&& conn) = default;

    bool operator==(const Connection& b) const;

    template <typename R, typename ...Args>
    Connection(Signal<R(Args...)>& sig, std::shared_ptr<void> ptr = nullptr)
        : pimpl(new ConnImpl<Signal<R(Args...)>>(sig, ptr)) {}

    void disconnect() const;
    bool isConnected() const;

private:
    template <typename,typename...> friend class Signal;
    friend struct ScopedConnection;
    friend struct ConnHash;
    std::shared_ptr<ConnErasure> pimpl;
};

struct ConnHash {
    size_t operator()(const Connection& conn) const;
};

struct ScopedConnection : Connection {
    ScopedConnection() = default;
    ScopedConnection(ScopedConnection&& conn) = default;
    ScopedConnection& operator=(ScopedConnection&& conn) = default;
    ScopedConnection(const Connection& conn);
    ScopedConnection(Connection&& conn);
    ScopedConnection& operator=(Connection&& conn);

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ~ScopedConnection();
};

}
}

#endif // MELOSIC_CONNECTION_HPP
