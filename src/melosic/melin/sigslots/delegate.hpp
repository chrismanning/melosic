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

#ifndef MELOSIC_DELEGATE_HPP
#define MELOSIC_DELEGATE_HPP

#include <functional>

template <typename T>
class Delegate;

template <typename Ret, typename ...Args>
class Delegate<Ret(Args...)> {
public:
    template <typename Obj>
    Delegate(const std::function<Ret, Args...>& de, Obj) {

    }

    Ret operator()(Args... args) {
        return fun(std::forward<Args>(args...));
    }

private:
    std::function<Ret(Args...)> fun;
    bool member = false;
};

#endif // MELOSIC_DELEGATE_HPP
