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

#include <boost/lockfree/queue.hpp>

#include <melosic/common/error.hpp>
#include <melosic/melin/logging.hpp>

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
        typedef typename std::result_of<Func()>::type Result;
        impl(std::promise<Result>&& p, Func&& f) : p(std::move(p)), f(std::move(f)) {}

        void call() {
            try {
                call_impl<typename std::result_of<Func()>::type>::call(*this);
            }
            catch(...) {
                p.set_exception(std::current_exception());
            }
        }

    private:
        template <typename T, typename D = void>
        struct call_impl {
            static void call(impl<Func>& i) {
                i.p.set_value(i.f());
            }
        };

        template <typename D>
        struct call_impl<void, D> {
            static void call(impl<Func>& i) {
                i.f();
                i.p.set_value();
            }
        };

        std::promise<Result> p;
        Func f;
    };

    impl_base* pimpl = nullptr;

public:
    Task() = default;

    void operator()() {
        if(pimpl)
            pimpl->call();
    }

    template <typename Func, typename ...Args>
    Task(std::promise<typename std::result_of<Func(Args...)>::type> p, Func&& f, Args&&... args) {
        typedef std::promise<typename std::result_of<Func(Args...)>::type> PromiseType;
        auto fun = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);
        pimpl = new impl<decltype(fun)>(std::move(p), std::move(fun));
    }

    void destroy() {
        if(pimpl) {
            delete pimpl;
            pimpl = nullptr;
        }
    }
};

class Manager {
public:
    Manager(const unsigned n = std::thread::hardware_concurrency() + 1);

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager();

    template <typename Func, typename ...Args>
    std::future<typename std::result_of<Func(Args...)>::type> enqueue(Func&& f, Args&&... args) {
        std::promise<typename std::result_of<Func(Args...)>::type> p;

        auto fut = p.get_future();

        Task t(std::move(p), std::move(f), std::forward<Args>(args)...);
        if(!tasks.bounded_push(t)) {
            BOOST_THROW_EXCEPTION(Exception());
        }

        return std::move(fut);
    }

    template <typename Func, typename ...Args>
    std::future<typename std::result_of<Func(Args...)>::type> enqueueSlot(Func&& f, Args&&... args) {
        std::promise<typename std::result_of<Func(Args...)>::type> p;

        auto fut = p.get_future();

        Task t(std::move(p), std::move(f), std::forward<Args>(args)...);
        if(!slots.bounded_push(t)) {
            BOOST_THROW_EXCEPTION(Exception());
        }

        return std::move(fut);
    }

    std::thread::id signalThreadId() {
        return signalThread.get_id();
    }

private:
    std::vector<std::thread> threads;
    std::thread signalThread;
    boost::lockfree::queue<Task> slots;
    boost::lockfree::queue<Task> tasks;
    std::atomic<bool> done;
    Logger::Logger logject;
};

} // namespace Thread
} // namespace Melosic

#endif // MELOSIC_THREAD_HPP
