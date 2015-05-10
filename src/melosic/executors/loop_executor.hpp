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

#include <deque>

#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>

namespace Melosic {
namespace executors {

enum class loop_options { try_yield, wait_sleep };

template <loop_options LoopMethod> struct basic_loop_executor {
    static_assert(LoopMethod == loop_options::try_yield || LoopMethod == loop_options::wait_sleep,
                  "Invalid LoopMethod");

    using work_type = boost::executors::work;

    basic_loop_executor() = default;

    ~basic_loop_executor() {
        m_destruct.store(true);
        m_work_queue.close();
        while(m_in_loop.test_and_set(std::memory_order_acquire))
            ;
        m_in_loop.clear(std::memory_order_release);
    }

    basic_loop_executor(const basic_loop_executor&) = delete;
    basic_loop_executor(basic_loop_executor&&) = delete;

  private:
    boost::sync_queue<work_type, std::deque<work_type>> m_work_queue;
    std::atomic_bool m_exit_loop{false};
    std::atomic_flag m_in_loop{ATOMIC_FLAG_INIT};
    std::atomic_bool m_destruct{false};

    void try_loop() {
        work_type work;
        while(!m_destruct.load() && !m_exit_loop.load(std::memory_order_relaxed) && !m_work_queue.closed()) {
            try {
                if(m_work_queue.try_pull(work) == boost::queue_op_status::success)
                    work();
                else
                    boost::this_thread::yield();
            } catch(boost::thread_interrupted&) {
                throw;
            } catch(...) {
                boost::this_thread::yield();
            }
        }
    }

    void wait_loop() {
        work_type work;
        while(!m_destruct.load() && !m_exit_loop.load(std::memory_order_relaxed) && !m_work_queue.closed()) {
            try {
                if(m_work_queue.wait_pull(work) == boost::queue_op_status::closed)
                    break;
                work();
            } catch(boost::thread_interrupted&) {
                throw;
            } catch(...) {
            }
        }
    }

  public:
    template <typename WorkT> void submit(WorkT&& work) {
        m_work_queue.push(work_type(std::forward<WorkT>(work)));
    }

    size_t uninitiated_task_count() const {
        return m_work_queue.size();
    }

    void loop() {
        while(m_in_loop.test_and_set(std::memory_order_acquire))
            ;

        m_exit_loop.store(false, std::memory_order_relaxed);
        try {
            if(LoopMethod == loop_options::try_yield)
                try_loop();
            else
                wait_loop();
        } catch(boost::thread_interrupted&) {
            m_in_loop.clear(std::memory_order_release);
            throw;
        }
        m_in_loop.clear(std::memory_order_release);
    }

    void run_queued_closures() {
        while(m_in_loop.test_and_set(std::memory_order_acquire))
            ;

        m_exit_loop.store(false, std::memory_order_relaxed);

        auto closures = m_work_queue.underlying_queue();
        for(auto&& work : closures) {
            if(m_exit_loop.load())
                return;
            try {
                work();
            } catch(boost::thread_interrupted&) {
                m_in_loop.clear(std::memory_order_release);
                throw;
            } catch(...) {
            }
        }
        m_in_loop.clear(std::memory_order_release);
    }

    bool try_run_one_closure() {
        try {
            work_type work;
            if(m_work_queue.try_pull(work) != boost::queue_op_status::success)
                return false;
            work();
            return true;
        } catch(boost::thread_interrupted&) {
            throw;
        } catch(...) {
            return false;
        }
    }

    void make_loop_exit() {
        m_exit_loop.store(true, std::memory_order_relaxed);
    }
};

using loop_executor = basic_loop_executor<loop_options::wait_sleep>;

} // namespace executors
} // namespace Melosic

#endif // MELOSIC_LOOP_EXECUTOR_HPP
