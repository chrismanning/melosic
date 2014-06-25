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
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/executors/loop_executor.hpp>
#include <boost/thread/executors/inline_executor.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/strict_lock.hpp>
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

class SignalEnv {
    SignalEnv()
        : m_thread(&boost::loop_executor::loop, &m_loop_executor),
          m_maint_thread(&boost::loop_executor::loop, &m_maint_executor) {}

    ~SignalEnv() {
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

    template <typename... Args>
    static auto wrap_slot(std::shared_ptr<SignalImpl<Args...>> sig_impl,
                          std::tuple<Connection, std::function<void(Args...)>>& tuple) {
        using std::get;
        return [ sig_impl, f = get<std::function<void(Args...)>>(tuple), conn = get<Connection>(tuple) ](
            Args... args) mutable {
            try {
                f(args...);
                return true;
            } catch(...) {
                instance().m_maint_executor.submit([=]() mutable { conn.disconnect(); });
                return false;
            }
        };
    }

    template <typename Executor, typename... Args, typename... CallArgs>
    static auto future_call(
        Executor& ex, std::shared_ptr<SignalImpl<Args...>> sig_impl,
        boost::strict_lock_ptr<std::vector<std::tuple<Connection, std::function<void(Args...)>>>, std::mutex>& funs,
        CallArgs&&... args) {
        std::vector<boost::future<bool>> futures;

        for(auto&& tuple : *funs)
            futures.push_back(boost::async(ex, wrap_slot(sig_impl, tuple), std::forward<CallArgs>(args)...));

        if(boost::this_thread::get_id() == instance().m_thread.get_id())
            return boost::async(instance().m_inline_executor, &SignalEnv::wait_for_all, std::move(futures));
        else
            return boost::async(instance().m_maint_executor, &SignalEnv::wait_for_all, std::move(futures));
    }

    template <typename Executor, typename... Args, typename... CallArgs>
    static void
    call(Executor& ex, std::shared_ptr<SignalImpl<Args...>> sig_impl,
         boost::strict_lock_ptr<std::vector<std::tuple<Connection, std::function<void(Args...)>>>, std::mutex>& funs,
         CallArgs&&... args) {
        for(auto&& tuple : *funs)
            ex.submit([ f = wrap_slot(sig_impl, tuple), args... ]() mutable { f(std::forward<CallArgs>(args)...); });
    }

  public:
    static SignalEnv& instance() {
        static SignalEnv impl;
        return impl;
    }

    template <typename... Args, typename... CallArgs>
    static auto future_call(
        std::shared_ptr<SignalImpl<Args...>> sig_impl,
        boost::strict_lock_ptr<std::vector<std::tuple<Connection, std::function<void(Args...)>>>, std::mutex> funs,
        CallArgs&&... args) {
        if(boost::this_thread::get_id() == instance().m_thread.get_id())
            return future_call(instance().m_inline_executor, sig_impl, funs, std::forward<CallArgs>(args)...);
        else
            return future_call(instance().m_loop_executor, sig_impl, funs, std::forward<CallArgs>(args)...);
    }

    template <typename... Args, typename... CallArgs>
    static void
    call(std::shared_ptr<SignalImpl<Args...>> sig_impl,
         boost::strict_lock_ptr<std::vector<std::tuple<Connection, std::function<void(Args...)>>>, std::mutex> funs,
         CallArgs&&... args) {
        if(boost::this_thread::get_id() == instance().m_thread.get_id())
            call(instance().m_inline_executor, sig_impl, funs, std::forward<CallArgs>(args)...);
        else
            call(instance().m_loop_executor, sig_impl, funs, std::forward<CallArgs>(args)...);
    }
};

template <typename... Args> class SignalImpl final : public std::enable_shared_from_this<SignalImpl<Args...>> {
    friend struct SignalCore<void(Args...)>;
    friend struct ConnImpl<Args...>;

    using mutex = std::mutex;
    using strict_lock = boost::strict_lock<mutex>;
    using slot_type = std::function<void(Args...)>;
    using slot_container_type = std::vector<std::tuple<Connection, slot_type>>;
    using synchronized_slot_container = boost::strict_lock_ptr<slot_container_type, mutex>;

    slot_container_type m_slots;
    mutex mu;

    size_t slotCount() {
        strict_lock l(mu);
        return m_slots.size();
    }

    Connection connect(const slot_type& slot) {
        strict_lock l(mu);
        return connect(slot, l);
    }

    Connection connect(slot_type&& slot) {
        strict_lock l(mu);
        return connect(std::move(slot), l);
    }

    void disconnect(const Connection& conn) {
        strict_lock l(mu);
        disconnect(conn, l);
    }

    bool connected(const Connection& conn) {
        strict_lock l(mu);
        return connected(conn, l);
    }

    template <typename... CallArgs> void call(CallArgs&&... args) {
        SignalEnv::call(this->shared_from_this(), synchronized_slot_container{m_slots, mu},
                        std::forward<CallArgs>(args)...);
    }

    template <typename... CallArgs> boost::future<void> future_call(CallArgs&&... args) {
        return SignalEnv::future_call(this->shared_from_this(), synchronized_slot_container{m_slots, mu},
                                      std::forward<CallArgs>(args)...);
    }

    // locked

    Connection connect(const slot_type& slot, strict_lock& l) { return connect(slot_type(slot), l); }
    Connection connect(slot_type&& slot, strict_lock&) {
        using std::get;
        m_slots.emplace_back(Connection(this->shared_from_this()), std::move(slot));
        return get<Connection>(m_slots.back());
    }

    void disconnect(const Connection& conn, strict_lock&) {
        using std::get;
        m_slots.erase(std::remove_if(m_slots.begin(), m_slots.end(),
                                     [&conn](auto&& tuple) { return get<Connection>(tuple) == conn; }),
                      m_slots.end());
    }

    bool connected(const Connection& conn, strict_lock&) {
        using std::get;
        return m_slots.end() != std::find_if(m_slots.begin(), m_slots.end(),
                                             [&conn](auto&& tuple) { return get<Connection>(tuple) == conn; });
    }
};

} // namespace detail

template <typename Ret, typename... Args> struct SignalCore<Ret(Args...)> {
    static_assert(std::is_void<Ret>::value, "Signals cannot return values.");
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

    using slot_type = typename detail::SignalImpl<Args...>::slot_type;

  private:
    std::shared_ptr<detail::SignalImpl<Args...>> pimpl;

  protected:
    SignalCore() : pimpl(std::make_shared<detail::SignalImpl<Args...>>()) {}

    SignalCore(const SignalCore&) = delete;
    SignalCore& operator=(const SignalCore&) = delete;

    SignalCore(SignalCore&& b) noexcept(std::is_nothrow_move_constructible<decltype(pimpl)>::value)
        : pimpl(std::move(b.pimpl)) {}

    SignalCore& operator=(SignalCore b) noexcept(
        std::is_nothrow_move_constructible<SignalCore>::value&& is_nothrow_swappable<SignalCore>::value) {
        swap(b);
        return *this;
    }

    ~SignalCore() {}

  public:
    Connection connect(slot_type slot) { return pimpl->connect(std::move(slot)); }

    template <typename MemRet, typename T, typename Obj, typename... As>
    Connection connect(MemRet (T::*const func)(As...), Obj&& obj) {
        static_assert(sizeof...(As) == sizeof...(Args), "");
        auto bo = bindObj(std::forward<Obj>(obj));
        static_assert(std::is_base_of<T, typename decltype(bo)::object_type>::value,
                      "obj must have member function func");
        return connect([=](Args&&... as) mutable {
            auto ptr = bo();
            (std::addressof(*ptr)->*func)(std::forward<detail::unwrap_t<Args>>(as)...);
        });
    }

    void disconnect(const Connection& conn) { pimpl->disconnect(conn); }

    bool connected(const Connection& conn) const { return pimpl->connected(conn); }

    size_t slotCount() const noexcept { return pimpl->slotCount(); }

    void swap(SignalCore& b) noexcept(is_nothrow_swappable<decltype(pimpl)>::value) {
        using std::swap;
        swap(pimpl, b.pimpl);
    }

  protected:
    template <typename... A> void call(A&&... args) { pimpl->call(std::forward<A>(args)...); }

    template <typename... A> boost::future<void> future_call(A&&... args) {
        return pimpl->future_call(std::forward<A>(args)...);
    }
};

template <typename Ret, typename... Args>
void swap(SignalCore<Ret(Args...)>& a, SignalCore<Ret(Args...)>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

} // namespace Signals
} // namespace Melosic

#endif // MELOSIC_SIGNAL_CORE_HPP
