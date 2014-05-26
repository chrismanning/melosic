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

#ifndef MELOSIC_SIGNAL_CORE_HPP
#define MELOSIC_SIGNAL_CORE_HPP

#include <unordered_map>
#include <future>
#include <mutex>
#include <functional>
#include <type_traits>

#include <boost/mpl/if.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/implicit_cast.hpp>

#include <melosic/common/traits.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/common/ptr_adaptor.hpp>
#include <melosic/common/connection.hpp>
#include <melosic/common/scope_unlock_exit_lock.hpp>

namespace Melosic {
namespace Signals {

namespace {
namespace mpl = boost::mpl;
}

namespace detail {

template <typename T>
struct unwrap {
    using type = T;
};

template <typename T>
struct unwrap<std::reference_wrapper<T>> {
    using type = T&;
};

template <typename T>
using unwrap_t = typename unwrap<T>::type;

}

template <typename Ret, typename ...Args>
struct SignalCore<Ret (Args...)> {
    static_assert(mpl::if_<mpl::bool_<(sizeof...(Args) > 0)>,
                  mpl::not_<MultiArgsTrait<std::is_rvalue_reference<mpl::_1>, Args...>>,
                  std::true_type>::type::value,
                  "Signal args cannot be rvalue references as they may be used more than once");
    static_assert(mpl::if_<mpl::bool_<(sizeof...(Args) > 0)>,
                  MultiArgsTrait<mpl::or_<std::is_copy_constructible<mpl::_1>,
                                          std::is_trivially_copyable<mpl::_1>>,
                                 typename std::decay<Args>::type...>,
                  std::true_type>::type::value,
                  "Signal args must be copyable");

    using Mutex = std::mutex;
    using lock_guard = std::lock_guard<Mutex>;
    using unique_lock = std::unique_lock<Mutex>;

    using Slot = std::function<Ret(Args...)>;
    using FunsType = std::unordered_map<Connection, Slot, ConnHash>;

    SignalCore() noexcept(std::is_nothrow_default_constructible<FunsType>::value
                          && std::is_nothrow_default_constructible<Mutex>::value) = default;

    SignalCore(const SignalCore&) = delete;
    SignalCore& operator=(const SignalCore&) = delete;

    SignalCore(SignalCore&& b)
            noexcept(std::is_nothrow_move_constructible<FunsType>::value
                     && std::is_nothrow_constructible<Mutex>::value)
        : funs(std::move(b.funs))
    {}

    SignalCore& operator=(SignalCore b)
            noexcept(std::is_nothrow_move_constructible<SignalCore>::value
                     && is_nothrow_swappable<SignalCore>::value)
    {
        using std::swap;
        swap(*this, b);
        return *this;
    }

    virtual ~SignalCore() {
        while(!funs.empty()) {
            auto c(funs.begin()->first);
            c.disconnect();
        }
    }

    Connection connect(Slot slot) {
        lock_guard l(mu);
        return funs.emplace(Connection(*this), slot).first->first;
    }

    template <typename T, typename Obj, typename... As>
    Connection connect(Ret (T::* const func)(As...), Obj&& obj) {
        static_assert(sizeof...(As) == sizeof...(Args), "");
        auto bo = bindObj(std::forward<Obj>(obj));
        static_assert(std::is_base_of<T, typename decltype(bo)::object_type>::value,
                      "obj must have member function func");
        return connect([=] (Args&&... as) mutable { return (bo()->*func)(std::forward<detail::unwrap_t<Args>>(as)...); });
    }

    bool disconnect(const Connection& conn) {
        lock_guard l(mu);
        return disconnect(conn, nullptr);
    }

private:
    bool disconnect(const Connection& conn, void* locked) {
        auto s = funs.size();
        auto n = funs.erase(conn);
        return funs.size() == s-n;
    }

public:
    size_t slotCount() const noexcept {
        return funs.size();
    }

    void swap(SignalCore& b) noexcept(is_nothrow_swappable<FunsType>::value) {
        unique_lock l{mu, std::defer_lock}, l2{b.mu, std::defer_lock};
        std::lock(l, l2);
        using std::swap;
        swap(funs, b.funs);
    }

protected:
    template <typename ...A>
    std::future<void> call(A&& ...args) {
        static Thread::Manager tman{1};

        std::promise<void> promise;
        auto ret(promise.get_future());
        std::list<std::pair<std::future<Ret>, Connection>> futures;

        std::exception_ptr eptr;

        unique_lock l(mu);
        for(auto i(funs.begin()); i != funs.end();) {
            try {
                if(!tman.contains(boost::this_thread::get_id())) {
                    try {
                        futures.emplace_back(std::move(tman.enqueue(i->second, std::forward<A>(args)...)), i->first);
                    }
                    catch(TaskQueueError& e) {
                        scope_unlock_exit_lock<unique_lock> s{l};
                        i->second(std::forward<A>(args)...);
                    }
                }
                else {
                    scope_unlock_exit_lock<unique_lock> s{l};
                    i->second(std::forward<A>(args)...);
                }
                ++i;
            }
            catch(std::bad_weak_ptr&) {
                auto tmp(i->first);
                ++i;
                disconnect(tmp, nullptr);
                if(i != funs.begin())
                    i = std::next(funs.begin(), std::distance(funs.begin(), i)-1);
            }
            catch(...) {
                eptr = std::current_exception();
                ++i;
            }
        }
        for(auto&& f : futures) {
            try {
                f.first.get();
            }
            catch(std::bad_weak_ptr&) {
                disconnect(f.second, nullptr);
            }
            catch(...) {
                eptr = std::current_exception();
            }
        }

        if(eptr)
            promise.set_exception(eptr);
        else
            promise.set_value();

        return std::move(ret);
    }

private:
    FunsType funs;

    Mutex mu;
};

template <typename Ret, typename ...Args>
void swap(SignalCore<Ret (Args...)>& a, SignalCore<Ret (Args...)>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

}
}

#endif // MELOSIC_SIGNAL_CORE_HPP
