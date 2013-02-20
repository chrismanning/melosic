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

#ifndef USER_HPP
#define USER_HPP

#include <memory>
#include <string>

#include <network/uri.hpp>

namespace LastFM {
class Service;

struct User {
public:
    User();
    User(std::weak_ptr<Service> lastserv, const std::string& username);
    User(std::weak_ptr<Service> lastserv, const std::string& username, const std::string& sessionKey);

    User(const User&) = delete;
    User& operator=(const User&) = delete;
    User(User&&);
    User& operator=(User&&);

    ~User();

    std::future<bool> getInfo();

    void authenticate();
    const std::string& getSessionKey() const;
    void setSessionKey(const std::string&);

    explicit operator bool();

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

}//namespace LastFM

#endif // USER_HPP
