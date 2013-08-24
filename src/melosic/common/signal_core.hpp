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

#include <melosic/common/traits.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/common/ptr_adaptor.hpp>
#include <melosic/common/connection.hpp>

namespace Melosic {
namespace Signals {

namespace {
namespace mpl = boost::mpl;

using Mutex = std::mutex;
using lock_guard = std::lock_guard<Mutex>;
using unique_lock = std::unique_lock<Mutex>;

template <typename Lock>
using LockNoExcept = std::is_nothrow_constructible<Lock, typename Lock::mutex_type>;
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

    template <typename T, typename Obj>
    Connection connect(Ret (T::* const func)(Args...), Obj&& obj) {
        auto bo = bindObj(std::forward<Obj>(obj));
        static_assert(std::is_base_of<T, typename decltype(bo)::object_type>::value,
                      "obj must have member function func");
        return connect([=] (Args&&... as) mutable { return (bo()->*func)(std::forward<Args>(as)...); });
    }

    bool disconnect(const Connection& conn) noexcept(LockNoExcept<lock_guard>::value) {
        lock_guard l(mu);
        auto s = funs.size();
        auto n = funs.erase(conn);
        return funs.size() == s-n;
    }

    size_t slotCount() const noexcept {
        return funs.size();
    }

    void swap(SignalCore& b) noexcept(is_nothrow_swappable<FunsType>::value) {
        std::lock(mu, b.mu);
        using std::swap;
        swap(funs, b.funs);
        b.mu.unlock();
        mu.unlock();
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
                l.unlock();
                if(!tman.contains(std::this_thread::get_id())) {
                    try {
                        futures.emplace_back(std::move(tman.enqueue(i->second, std::forward<A>(args)...)), i->first);
                    }
                    catch(TaskQueueError& e) {
                        i->second(std::forward<Args>(args)...);
                    }
                }
                else
                    i->second(std::forward<Args>(args)...);
                l.lock();
                ++i;
            }
            catch(std::bad_weak_ptr&) {
                auto tmp(i->first);
                ++i;
                tmp.disconnect();
                l.lock();
                if(i != funs.begin())
                    i = std::next(funs.begin(), std::distance(funs.begin(), i)-1);
            }
            catch(...) {
                eptr = std::current_exception();
                l.lock();
                ++i;
            }
        }
        for(auto&& f : futures) {
            try {
                l.unlock();
                f.first.get();
                l.lock();
            }
            catch(std::bad_weak_ptr&) {
                f.second.disconnect();
            }
            catch(...) {
                eptr = std::current_exception();
                l.lock();
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
