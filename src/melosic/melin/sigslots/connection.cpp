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

#include "connection.hpp"

namespace Melosic {
namespace Signals {

size_t ConnHash::operator()(const Connection& conn) const {
    return std::hash<std::shared_ptr<ConnErasure>>()(conn.pimpl);
}

bool Connection::operator==(const Connection& b) const {
    return pimpl == b.pimpl;
}

void Connection::disconnect() const {
    pimpl->disconnect(*this);
}

bool Connection::isConnected() const {
    return pimpl->isConnected();
}

ScopedConnection::~ScopedConnection() {
    if(pimpl)
        disconnect();
}

ScopedConnection::ScopedConnection(const Connection& conn) {
    pimpl = conn.pimpl;
}

ScopedConnection::ScopedConnection(Connection&& conn) : Connection(conn) {}

ScopedConnection& ScopedConnection::operator=(Connection&& conn) {
    pimpl = conn.pimpl;
    conn.pimpl.reset();

    return *this;
}

}
}
