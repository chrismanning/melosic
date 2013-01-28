/**************************************************************************
**  Copyright (C) 2012 Christian Manning
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

#include <string>

#include "user.hpp"

class User::impl {
public:
    impl(const std::string& username) : username(username) {

    }
    impl(const std::string& username, const std::string& sessionKey)
        : username(username),
          sessionKey(sessionKey)
    {

    }

private:
    const std::string& username;
    std::string sessionKey;
};

User::User(const std::string& username) : pimpl(new impl(username)) {}

User::User(const std::string& username, const std::string& sessionKey)
    : pimpl(new impl(username, sessionKey))
{
}
