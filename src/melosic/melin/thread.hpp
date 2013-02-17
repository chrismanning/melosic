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
        impl(std::promise<Result>& p, Func&& f) : p(p), f(std::move(f)) {}
        impl(std::promise<Result>& p, const Func& f) : p(p), f(f) {}

        void call() {
            try {
                call_impl(typename std::is_same<typename std::result_of<Func()>::type, void>::type());
            }
            catch(...) {
                p.set_exception(std::current_exception());
            }
        }

    private:
        void call_impl(std::true_type&&) {
            f();
        }
        void call_impl(std::false_type&&) {
            p.set_value(f());
        }

        std::promise<Result>& p;
        Func f;
    };

    impl_base* pimpl = nullptr;

public:
    Task() {}

    void operator()() {
        if(pimpl)
            pimpl->call();
    }

    template <typename Func, typename ...Args>
    Task(std::promise<typename std::result_of<Func(Args...)>::type>& p, Func&& f, Args&&... args) {
        typedef decltype(std::bind(f, std::forward<Args>(args)...)) FT;
        pimpl = new impl<FT>(p, std::bind(f, std::forward<Args>(args)...));
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
    Manager() : tasks(100), done(false), logject(logging::keywords::channel = "Thread::Manager") {
        const unsigned n = std::thread::hardware_concurrency() * 2;
        for(unsigned i=0; i<n; i++)
            threads.emplace_back([this]() {
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
            });
        LOG(logject) << threads.size() << " threads started in thread pool";
    }

    ~Manager() {
        done = true;
        for(auto& t : threads)
            t.join();
    }

    template <typename Func, typename ...Args>
    std::future<typename std::result_of<Func(Args...)>::type> enqueue(Func&& f, Args&&... args) {
        std::promise<typename std::result_of<Func(Args...)>::type> p;

        TRACE_LOG(logject) << "Adding task to thread pool with type: " << typeid(f);

        Task t(p, std::move(f), std::forward<Args>(args)...);
        if(!tasks.push(t)) {
            BOOST_THROW_EXCEPTION(Exception());
        }

        return p.get_future();
    }

    template <typename Func, typename ...Args>
    std::future<typename std::result_of<Func(Args...)>::type> enqueue(const Func& f, Args&&... args) {
        std::promise<typename std::result_of<Func(Args...)>::type> p;

        TRACE_LOG(logject) << "Adding task to thread pool with type: " << typeid(f);

        Task t(p, f, std::forward<Args>(args)...);
        if(!tasks.push(t)) {
            BOOST_THROW_EXCEPTION(Exception());
        }

        return p.get_future();
    }

private:
    std::vector<std::thread> threads;
    boost::lockfree::queue<Task> tasks;
    std::atomic<bool> done;
    Logger::Logger logject;
};

} // namespace Thread
} // namespace Melosic

#endif // MELOSIC_THREAD_HPP
