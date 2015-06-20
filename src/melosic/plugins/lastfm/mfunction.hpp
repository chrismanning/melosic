/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#ifndef MFUNCTION_HPP
#define MFUNCTION_HPP

#include <memory>

namespace util {

template <typename Ret, typename... Args>
class mfunction;

template <typename Ret, typename... Args>
class mfunction<Ret(Args...)> {
    struct function_base {
        virtual Ret invoke(Args...) const = 0;
    };
    std::unique_ptr<function_base> fun_base;

    template <typename Fun>
    struct function_impl : function_base {
        using function_t = std::decay_t<Fun>;

        template <typename Fun_>
        function_impl(Fun_&& fun) : fun(std::forward<Fun_>(fun)) {}

        Ret invoke(Args... args) const override {
            return fun(std::forward<Args>(args)...);
        }
        mutable function_t fun;
    };

  public:
    mfunction() noexcept = default;
    mfunction(const mfunction&) = delete;
    mfunction(mfunction&&) noexcept = default;

    template <typename Fun>
    mfunction(Fun&& fun) : fun_base(std::make_unique<function_impl<Fun>>(std::forward<Fun>(fun))) {}

    Ret operator()(Args... args) const {
        assert(fun_base);
        return std::const_pointer_cast<const function_base>(fun_base)->invoke(std::forward<Args>(args)...);
    }

    Ret operator()(Args... args) {
        assert(fun_base);
        return fun_base->invoke(std::forward<Args>(args)...);
    }

    explicit operator bool() const {
        return static_cast<bool>(fun_base);
    }
};

} // namespace util

#endif // MFUNCTION_HPP

