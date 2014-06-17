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
#include <boost/thread/synchronized_value.hpp>

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

template <typename T> struct unwrap { using type = T; };

template <typename T> struct unwrap<std::reference_wrapper<T>> { using type = T&; };

template <typename T> using unwrap_t = typename unwrap<T>::type;

class SignalImpl {
    SignalImpl()
        : m_thread(&boost::loop_executor::loop, &m_loop_executor),
          m_maint_thread(&boost::loop_executor::loop, &m_maint_executor) {}

    ~SignalImpl() {
        m_loop_executor.close();
        m_maint_executor.close();
    }

    boost::loop_executor m_loop_executor;
    boost::inline_executor m_inline_executor;
    boost::scoped_thread<> m_thread;
    boost::loop_executor m_maint_executor;
    boost::scoped_thread<> m_maint_thread;

    static void wait_for_all(std::vector<boost::future<bool>>&& futures) {
        for(auto&& f : futures) {
            if(f.is_ready())
                continue;
            f.wait();
        }
    }

    template <typename Executor, typename... Args, typename... CallArgs>
    static auto call(Executor& ex,
                     boost::strict_lock_ptr<std::vector<std::tuple<Connection, std::function<void(Args...)>>>>& funs,
                     CallArgs&&... args) {
        using std::get;

        std::vector<boost::future<bool>> futures;

        for(auto&& tuple : *funs) {
            auto wrapped_fun = [ f = get<std::function<void(Args...)>>(tuple), conn = get<Connection>(tuple) ](
                Args... args) mutable {
                try {
                    f(args...);
                    return true;
                } catch(...) {
                    instance().m_maint_executor.submit([=]() mutable { conn.disconnect(); });
                    return false;
                }
            };
            futures.push_back(boost::async(ex, std::move(wrapped_fun), std::forward<CallArgs>(args)...));
        }

        if(boost::this_thread::get_id() == instance().m_thread.get_id())
            return boost::async(instance().m_inline_executor, &SignalImpl::wait_for_all, std::move(futures));
        else
            return boost::async(instance().m_maint_executor, &SignalImpl::wait_for_all, std::move(futures));
    }

  public:
    static SignalImpl& instance() {
        static SignalImpl impl;
        return impl;
    }

    template <typename... Args, typename... CallArgs>
    static auto call(boost::strict_lock_ptr<std::vector<std::tuple<Connection, std::function<void(Args...)>>>> funs,
                     CallArgs&&... args) {
        if(boost::this_thread::get_id() == instance().m_thread.get_id())
            return call(instance().m_inline_executor, funs, std::forward<CallArgs>(args)...);
        else
            return call(instance().m_loop_executor, funs, std::forward<CallArgs>(args)...);
    }
};

} // namespace detail

template <typename Ret, typename... Args> struct SignalCore<Ret(Args...)> {
    static_assert(
        mpl::if_<mpl::bool_<(sizeof...(Args) > 0)>,
                 mpl::not_<MultiArgsTrait<std::is_rvalue_reference<mpl::_1>, Args...>>, std::true_type>::type::value,
        "Signal args cannot be rvalue references as they may be used more than once");
    static_assert(
        mpl::if_<mpl::bool_<(sizeof...(Args) > 0)>,
                 MultiArgsTrait<mpl::or_<std::is_copy_constructible<mpl::_1>, std::is_trivially_copyable<mpl::_1>>,
                                typename std::decay<Args>::type...>,
                 std::true_type>::type::value,
        "Signal args must be copyable");

    using Slot = std::function<Ret(Args...)>;

  private:
    using FunsType = boost::synchronized_value<std::vector<std::tuple<Connection, Slot>>>;

  public:
    SignalCore() noexcept(std::is_nothrow_default_constructible<FunsType>::value) = default;

    SignalCore(const SignalCore&) = delete;
    SignalCore& operator=(const SignalCore&) = delete;

    SignalCore(SignalCore&& b) noexcept(std::is_nothrow_move_constructible<FunsType>::value)
        : funs(std::move(b.funs)) {}

    SignalCore& operator=(SignalCore b) noexcept(
        std::is_nothrow_move_constructible<SignalCore>::value&& is_nothrow_swappable<SignalCore>::value) {
        swap(b);
        return *this;
    }

    virtual ~SignalCore() {
        using std::get;
        while(!funs->empty()) {
            Connection c{get<Connection>(funs->back())};
            c.disconnect();
        }
    }

    Connection connect(Slot slot) {
        using std::get;
        auto s_funs = funs.synchronize();
        s_funs->emplace_back(Connection(*this), slot);
        return get<Connection>(s_funs->back());
    }

    template <typename T, typename Obj, typename... As> Connection connect(Ret (T::*const func)(As...), Obj&& obj) {
        static_assert(sizeof...(As) == sizeof...(Args), "");
        auto bo = bindObj(std::forward<Obj>(obj));
        static_assert(std::is_base_of<T, typename decltype(bo)::object_type>::value,
                      "obj must have member function func");
        return connect([=](Args&&... as) mutable {
            auto ptr = bo();
            return (std::addressof(*ptr)->*func)(std::forward<detail::unwrap_t<Args>>(as)...);
        });
    }

    bool disconnect(const Connection& conn) { return disconnect(conn, funs.synchronize()); }

  private:
    static bool disconnect(const Connection& conn, decltype(std::declval<FunsType>().synchronize()) s_funs) {
        auto s = s_funs->size();
        s_funs->erase(std::remove_if(s_funs->begin(), s_funs->end(), [&conn](auto&& tuple) {
                          using std::get;
                          return get<Connection>(tuple) == conn;
                      }),
                      s_funs->end());
        return s_funs->size() < s;
    }

  public:
    size_t slotCount() const noexcept { return funs->size(); }

    void swap(SignalCore& b) noexcept(is_nothrow_swappable<FunsType>::value) {
        using std::swap;
        swap(funs, b.funs);
    }

  protected:
    template <typename... A> boost::future<void> call(A&&... args) {
        return detail::SignalImpl::call(funs.synchronize(), std::forward<A>(args)...);
    }

  private:
    FunsType funs;
};

template <typename Ret, typename... Args>
void swap(SignalCore<Ret(Args...)>& a, SignalCore<Ret(Args...)>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}
}
}

#endif // MELOSIC_SIGNAL_CORE_HPP
