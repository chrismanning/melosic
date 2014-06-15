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
#include <boost/thread/reverse_lock.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/executors/loop_executor.hpp>
#include <boost/thread/executors/inline_executor.hpp>
#include <boost/thread/future.hpp>

#include <melosic/common/traits.hpp>
#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/common/ptr_adaptor.hpp>
#include <melosic/common/connection.hpp>

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

struct join_for_and_interrupt : boost::interrupt_and_join_if_joinable {
    void operator()(boost::thread& t) {
        if(t.joinable() && !t.try_join_for(boost::chrono::milliseconds(1000)))
            boost::interrupt_and_join_if_joinable::operator()(t);
    }
};

class SignalImpl {
    SignalImpl() : m_thread(&boost::loop_executor::loop, &m_loop_executor) {}

    ~SignalImpl() {
        m_loop_executor.close();
    }

    boost::loop_executor m_loop_executor;
    boost::inline_executor m_inline_executor;
    boost::scoped_thread<join_for_and_interrupt> m_thread;

    template <typename ...Args>
    static auto create_wrapper(const std::function<void(Args...)>& fun, Connection conn) {
        return [=](Args... args) mutable {
            try {
                fun(args...);
                return true;
            }
            catch(...) {
                conn.disconnect();
                return false;
            }
        };
    }

    template <typename Executor, typename ...Args, typename ...CallArgs>
    static boost::future<bool> exec_one(Executor& ex, const std::function<void(Args...)>& fun, const Connection& conn,
                                        CallArgs&&... args) {
        return boost::async(ex, create_wrapper(fun, conn), std::forward<CallArgs>(args)...);
    }

    template <typename ...Args, typename ...CallArgs>
    static boost::future<bool> exec_one(const std::function<void(Args...)>& fun, const Connection& conn,
                                        CallArgs&&... args) {
        if(boost::this_thread::get_id() == instance().m_thread.get_id())
            return exec_one(instance().m_inline_executor, fun, conn, std::forward<CallArgs>(args)...);
        else
            return exec_one(instance().m_loop_executor, fun, conn, std::forward<CallArgs>(args)...);
    }

public:
    static SignalImpl& instance() {
        static SignalImpl impl;
        return impl;
    }

    template <typename ...Args, typename ...CallArgs>
    static auto call(std::vector<std::tuple<Connection, std::function<void(Args...)>>> funs,
                     CallArgs&&... args) {
        using std::get;

        std::vector<boost::future<bool>> futures;

        for(auto&& tuple : funs) {
            futures.push_back(exec_one(get<std::function<void(Args...)>>(tuple), get<Connection>(tuple),
                                       std::forward<CallArgs>(args)...));
        }

        return boost::when_all(futures.begin(), futures.end());
    }
};

} // namespace detail

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
    using unique_lock = boost::unique_lock<Mutex>;

    using Slot = std::function<Ret(Args...)>;
    using FunsType = std::vector<std::tuple<Connection, Slot>>;

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
        using std::get;
        while(!funs.empty()) {
            Connection c{get<Connection>(funs.back())};
            c.disconnect();
        }
    }

    Connection connect(Slot slot) {
        using std::get;
        lock_guard l(mu);
        funs.emplace_back(Connection(*this), slot);
        return get<Connection>(funs.back());
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
    bool disconnect(const Connection& conn, void* /*locked*/) {
        auto s = funs.size();
        funs.erase(std::remove_if(funs.begin(), funs.end(), [&conn](auto&& tuple) {
            using std::get;
            return get<Connection>(tuple) == conn;
        }), funs.end());
        return funs.size() < s;
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
    boost::future<void> call(A&& ...args) {
        detail::SignalImpl::call(funs, std::forward<A>(args)...);
        return boost::make_ready_future();
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
