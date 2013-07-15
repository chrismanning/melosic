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
#include <melosic/common/common.hpp>

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
        static_assert(std::is_bind_expression<Func>::value, "Func should be a bind expression");
        typedef typename std::result_of<Func()>::type Result;
        impl(std::promise<Result>&& p, Func&& f) : p(std::move(p)), f(std::move(f)) {}

        void call() {
            try {
                call_impl<Result>::call(*this);
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
    Task(std::promise<typename std::result_of<Func(Args...)>::type> p, const Func& f, Args&&... args) {
        auto fun = std::bind(f, std::forward<Args>(args)...);
        pimpl = new impl<decltype(fun)>(std::move(p), std::move(fun));
    }

    void destroy() {
        if(pimpl) {
            delete pimpl;
            pimpl = nullptr;
        }
    }
};

namespace {
using TaskQueue = boost::lockfree::queue<Task>;
}

class Manager {
public:
    explicit Manager(const unsigned n = std::thread::hardware_concurrency() + 1) :
        tasks(10),
        done(false)
    {
        assert(n > 0);
        auto f = [&](boost::lockfree::queue<Task>& tasks) {
            while(!done) {
                Task t;
                if(tasks.pop(t)) {
                    t();
                    t.destroy();
                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    std::this_thread::yield();
                }
            }
        };

        for(unsigned i=0; i<n; i++)
            threads.emplace_back(f, std::ref(tasks));
    }

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager() {
        done = true;
        for(auto& t : threads)
            t.join();
    }

    template <typename Func, typename ...Args>
    std::future<typename std::result_of<Func(Args...)>::type> enqueue(const Func& f, Args&&... args) {
        std::promise<typename std::result_of<Func(Args...)>::type> p;

        auto fut(p.get_future());

        Task t(std::move(p), f, std::forward<Args>(args)...);
        if(!tasks.bounded_push(t)) {
            p.set_exception(std::make_exception_ptr(Exception()));
        }

        return std::move(fut);
    }

    bool contains(std::thread::id id) const {
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
