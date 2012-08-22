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

#ifndef MELOSIC_ERROR_HPP
#define MELOSIC_ERROR_HPP

#include <exception>
#include <utility>

namespace Melosic {

template <class Exception, typename ... Args>
void enforceEx(bool expression, Args&& ... arguments) {
    if(!expression) {
        throw Exception(std::forward<Args>(arguments)...);
    }
}

template <class Exception, class DyException, typename ... Args>
void enforceEx(bool expression, Args&& ... arguments) {
    if(!expression) {
        throw static_cast<Exception>(DyException(std::forward<Args>(arguments)...));
    }
}

struct Exception : public std::exception {
    Exception(const char * msg) : msg(msg) {}
    Exception(const std::function<const char *()>& lazyStr) : msg(lazyStr()) {}

    virtual const char * what() const throw() {
        return msg;
    }

    const char * msg;
};

}

#endif // MELOSIC_ERROR_HPP
