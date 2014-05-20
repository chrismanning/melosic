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

#ifndef MELOSIC_THREAD_HPP
#define MELOSIC_THREAD_HPP

#include <memory>
#include <future>
#include <thread>
#include <vector>
#include <atomic>
#include <type_traits>
#include <chrono>
using namespace std::literals;

#include <boost/lockfree/queue.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/thread/thread.hpp>

#include <asio/io_service.hpp>

#include <melosic/common/error.hpp>
#include <melosic/common/traits.hpp>
#include <melosic/common/ptr_adaptor.hpp>

namespace Melosic {
namespace Thread {

struct Task {
private:
    struct impl_base {
        virtual ~impl_base() {}
        virtual void call() = 0;
    };

    template <typename Func>
    struct impl : impl_base {
        typedef typename std::result_of<Func()>::type Result; //Func must be callable
        impl(std::promise<Result> p, Func&& f)
        noexcept(std::is_nothrow_constructible<Func, decltype(std::forward<Func>(f))>::value)
            :
              p(std::move(p)), f(std::forward<Func>(f)) {}

        void call() noexcept override {
            try {
                call_impl();
            }
            catch(...) {
                p.set_exception(std::current_exception());
            }
        }

    private:
        template <typename T = Result>
        void call_impl(typename std::enable_if<!std::is_void<T>::value, T>::type* = 0) {
            p.set_value(f());
        }

        template <typename T = Result>
        void call_impl(typename std::enable_if<std::is_void<T>::value, T>::type* = 0) {
            f();
            p.set_value();
        }

        std::promise<Result> p;
        Func f;
    };

    impl_base* pimpl;

public:
    Task() noexcept : pimpl(nullptr) {}

    template <typename Ret, typename Base, typename Derived, typename ...Args, typename ...Args2>
    Task(std::promise<Ret> p, Ret (Base::* const f)(Args2...), Derived&& obj, Args&&... args) :
        Task(std::move(p),
             [=] (Args&&... as) { return (bindObj<keep_alive>(obj)()->*f)(std::forward<Args>(as)...); },
             std::forward<Args>(args)...)
    {}

    template <typename Ret, typename Func, typename ...Args>
    Task(std::promise<Ret> p, Func&& f, Args&&... args) :
        Task(std::move(p), [&] () { return f(std::forward<Args>(args)...); })
    {}

    template <typename Ret, typename Func>
    Task(std::promise<Ret> p, Func&& f) {
        static_assert(std::is_same<Ret, typename std::result_of<Func()>::type>::value, "");
        pimpl = new impl<Func>(std::move(p), std::forward<Func>(f));
        assert(pimpl);
    }

    void operator()() noexcept {
        try {
            if(pimpl != nullptr)
                pimpl->call();
        }
        catch(std::future_error& e) {
            assert(e.code() != std::future_errc::no_state);
        }
    }

    void destroy() noexcept {
        delete pimpl;
        pimpl = nullptr;
    }
};

namespace {
using TaskQueue = boost::lockfree::queue<Task>;
namespace mpl = boost::mpl;
}

class Manager {
public:
    Manager(size_t numThreads, TaskQueue::size_type numTasks,
            asio::io_service* io_service) :
        tasks(numTasks),
        asio_service(io_service)
    {
        assert(numThreads > 0);

        auto f = [this] () {
            while(!done.load()) {
                Task t;
                if(tasks.pop(t)) {
                    t();
                    t.destroy();
                }
                else {
                    if(asio_service && !asio_service->stopped()) {
                        std::error_code ec;
                        auto n = asio_service->poll_one(ec);
                        if(n == 0) {
                        }
                    }
                    std::this_thread::sleep_for(10ms);
                    std::this_thread::yield();
                }
            }
        };
        for(size_t i=0; i<numThreads; i++)
            threads.emplace_back(f);
        if(asio_service)
            threads.emplace_back([this] () {
                std::error_code ec;
                asio_service->run(ec);
            });
    }

    explicit Manager(size_t numThreads = std::thread::hardware_concurrency() + 1,
                     asio::io_service* io_service = nullptr) :
        Manager(numThreads, numThreads * 2, io_service)
    {}

    explicit Manager(asio::io_service* io_service) :
        Manager(std::thread::hardware_concurrency() + 1, io_service)
    {}

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager() {
        done.store(true);
        for(auto& t : threads)
            if(t.joinable() && !t.try_join_for(boost::chrono::milliseconds(1000)))
                t.interrupt();
    }

    //throws: std::future_error, TaskQueueError, Task *may* throw
    template <typename Func, typename ...Args, typename Ret = typename std::result_of<Func(Args...)>::type>
    std::future<Ret> enqueue(Func&& f, Args&&... args) {
        static_assert(mpl::if_<mpl::bool_<(sizeof...(Args) > 0)>,
                      MultiArgsTrait<mpl::or_<std::is_copy_constructible<mpl::_1>,
                                              std::is_trivially_copyable<mpl::_1>>,
                                     typename std::decay<Args>::type...>,
                      std::true_type>::type::value,
                      "Task args must be copyable");
        std::promise<Ret> p;

        auto fut(p.get_future());
        assert(fut.valid());

        Task t(std::move(p), std::forward<Func>(f), std::forward<Args>(args)...);
        if(!tasks.push(t)) {
            BOOST_THROW_EXCEPTION(TaskQueueError());
        }

        return std::move(fut);
    }

    bool contains(boost::thread::id id) const noexcept {
        for(auto&& t : threads)
            if(t.get_id() == id)
                return true;

        return false;
    }

private:
    std::vector<boost::thread> threads;
    TaskQueue tasks;
    std::atomic_bool done{false};
    asio::io_service* const asio_service;
};

} // namespace Thread
} // namespace Melosic

#endif // MELOSIC_THREAD_HPP
