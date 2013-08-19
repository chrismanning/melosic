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

#include <melosic/common/error.hpp>
#include <melosic/common/traits.hpp>

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
        impl(std::promise<Result> p, Func&& f) noexcept(noexcept(Func(std::forward<Func>(f)))) :
            p(std::move(p)), f(std::forward<Func>(f)) {}

        void call() noexcept override {
            try {
                call_impl(*this);
            }
            catch(...) {
                p.set_exception(std::current_exception());
            }
        }

    private:
        template <typename T = Result>
        void call_impl(impl<Func>& i, typename std::enable_if<!std::is_void<T>::value, T>::type* = 0) {
            i.p.set_value(i.f());
        }

        template <typename T = Result>
        void call_impl(impl<Func>& i, typename std::enable_if<std::is_void<T>::value, T>::type* = 0) {
            i.f();
            i.p.set_value();
        }

        std::promise<Result> p;
        Func f;
    };

    impl_base* pimpl;

    template <typename Func, typename Ret = typename std::result_of<Func()>::type>
    static impl_base* createImpl(std::promise<Ret> p, Func&& fun) {
        return new impl<Func>(std::move(p), std::forward<Func>(fun));
    }

public:
    Task() noexcept : pimpl(nullptr) {}

    void operator()() noexcept {
        if(pimpl)
            pimpl->call();
    }

    template <typename Func, typename ...Args, typename Ret = typename std::result_of<Func(Args...)>::type>
    Task(std::promise<Ret> p, Func&& f, Args&&... args)
    noexcept(noexcept(createImpl(std::move(p), std::bind(std::forward<Func>(f), std::forward<Args>(args)...)))) {
        pimpl = createImpl(std::move(p), std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
    }

    template <typename Func, typename Ret = typename std::result_of<Func()>::type>
    Task(std::promise<Ret> p, Func&& f)
    noexcept(noexcept(createImpl(std::move(p), std::forward<Func>(f)))) {
        pimpl = createImpl(std::move(p), std::forward<Func>(f));
        assert(pimpl);
    }

    void destroy() noexcept {
        delete pimpl;
        pimpl = nullptr;
    }
};

namespace {
using TaskQueue = boost::lockfree::queue<Task, boost::lockfree::fixed_sized<true>>;
namespace mpl = boost::mpl;
}

class Manager {
public:
    Manager(size_t numThreads, TaskQueue::size_type numTasks) :
        tasks(numTasks),
        done(false)
    {
        assert(numThreads > 0);

        auto f = [&] () {
            while(!done) {
                Task t;
                if(tasks.pop(t)) {
                    t();
                    t.destroy();
                }
                else {
                    std::this_thread::sleep_for(10ms);
                    std::this_thread::yield();
                }
            }
        };
        for(size_t i=0; i<numThreads; i++)
            threads.emplace_back(f);
    }

    explicit Manager(size_t numThreads = std::thread::hardware_concurrency() + 1) :
        Manager(numThreads, numThreads * 2)
    {}

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager() {
        done = true;
        for(auto& t : threads)
            if(t.joinable())
                t.join();
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

        Task t(std::move(p), std::forward<Func>(f), std::forward<Args>(args)...);
        if(!tasks.bounded_push(t)) {
            BOOST_THROW_EXCEPTION(TaskQueueError());
        }

        return std::move(fut);
    }

    bool contains(std::thread::id id) const noexcept {
        for(auto&& t : threads)
            if(t.get_id() == id)
                return true;

        return false;
    }

private:
    std::vector<std::thread> threads;
    TaskQueue tasks;
    std::atomic_bool done;
};

} // namespace Thread
} // namespace Melosic

#endif // MELOSIC_THREAD_HPP
