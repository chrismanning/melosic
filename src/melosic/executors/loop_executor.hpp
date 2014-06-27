/**************************************************************************
**  Copyright (C) 2014 Christian Manning
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

#ifndef MELOSIC_LOOP_EXECUTOR_HPP
#define MELOSIC_LOOP_EXECUTOR_HPP

#include <functional>

#include <boost/thread/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>

namespace Melosic {
namespace executors {

enum class loop_options {
    try_yield,
    wait_sleep
};

enum class shutdown_options {
    exhaust,
    abandon
};

template <loop_options LoopMethod, shutdown_options ShutdownMethod>
struct basic_loop_executor {
    static_assert(LoopMethod == loop_options::try_yield || LoopMethod == loop_options::wait_sleep,
                  "Invalid LoopMethod");
    static_assert(ShutdownMethod == shutdown_options::exhaust || ShutdownMethod == shutdown_options::abandon,
                  "Invalid ShutdownMethod");

    using work_type = boost::executors::work;

    basic_loop_executor() = default;

    basic_loop_executor(const basic_loop_executor&) = delete;
    basic_loop_executor(basic_loop_executor&&) = delete;

    ~basic_loop_executor() {
        m_work_queue.close();
    }

private:
    boost::sync_queue<work_type> m_work_queue;
    std::atomic_bool m_exit{false};

    void try_loop() {
        work_type work;
        while(!m_exit.load() && !m_work_queue.closed()) {
            try {
                if(m_work_queue.try_pull_front(work) == boost::queue_op_status::success) {
//                    assert(work);
                    work();
                }
                else
                    boost::this_thread::yield();
            }
            catch(boost::thread_interrupted&) {
                throw;
            }
            catch(...) {
                boost::this_thread::yield();
            }
        }
    }

    void wait_loop() {
        work_type work;
        while(!m_exit.load() && !m_work_queue.closed()) {
            try {
                if(m_work_queue.wait_pull_front(work) == boost::queue_op_status::closed)
                    break;
//                assert(work);
                work();
            }
            catch(boost::thread_interrupted&) {
                throw;
            }
            catch(...) {}
        }
    }

public:
    template <typename WorkT>
    void submit(WorkT&& work) {
        m_work_queue.push_back(work_type(std::forward<WorkT>(work)));
    }
//    void submit(work_type work) {
//        m_work_queue.push_back(std::move(work));
//    }

    void loop() {
        m_exit.store(false);
        if(LoopMethod == loop_options::try_yield)
            try_loop();
        else
            wait_loop();
        if(ShutdownMethod == shutdown_options::exhaust)
            run_queued_closures();
    }

    void run_queued_closures() {
        m_exit.store(false);
        auto closures = m_work_queue.underlying_queue();
        for(auto&& work : closures) {
            if(m_exit.load())
                return;
            try {
//                assert(work);
                work();
            }
            catch(boost::thread_interrupted&) {
                throw;
            }
            catch(...) {}
        }
    }

    bool try_run_one_closure() {
        try {
            work_type work;
            if(m_work_queue.try_pull_front(work) != boost::queue_op_status::success)
                return false;
//            assert(work);
            work();
            return true;
        }
        catch(boost::thread_interrupted&) {
            throw;
        }
        catch(...) {
            return false;
        }
    }

    void make_loop_exit() {
        m_exit.store(true);
    }
};

using loop_executor = basic_loop_executor<loop_options::wait_sleep, shutdown_options::exhaust>;

} // namespace executors
} // namespace Melosic

#endif // MELOSIC_LOOP_EXECUTOR_HPP
