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

namespace LastFM {

class User {
public:
    explicit User(const std::string& username);
    User(const std::string& username, const std::string& sessionKey);

    User(const User&);
    User(User&&);

    void authenticate();
    const std::string& getSessionKey();
    void setSessionKey(const std::string&);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}//namespace LastFM

#endif // USER_HPP
