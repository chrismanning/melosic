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

#ifndef MELOSIC_EXECUTORS_THREAD_POOL_HPP
#define MELOSIC_EXECUTORS_THREAD_POOL_HPP

#include <functional>

#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>

namespace Melosic {
namespace executors {

struct thread_pool {
    using work_type = boost::executors::work;

    explicit thread_pool(unsigned thread_count = boost::thread::hardware_concurrency()) {
        if(thread_count == 0)
            m_work_queue.close();
        for(unsigned i = 0; i < thread_count; i++)
            m_threads.emplace_back([this]() {
                work_type work;
                while(!m_work_queue.closed()) {
                    try {
                        if(m_work_queue.wait_pull_front(work) == boost::queue_op_status::closed)
                            break;
                        work();
                    }
                    catch(boost::thread_interrupted&) {
                        throw;
                    }
                    catch(...) {}
                }
                while(true) {
                    try {
                        if(m_work_queue.try_pull_front(work) != boost::queue_op_status::success)
                            break;
                        work();
                    }
                    catch(boost::thread_interrupted&) {
                        throw;
                    }
                    catch(...) {}
                }
            });
    }

    ~thread_pool() {
        m_work_queue.close();
    }

    template <typename WorkT>
    void submit(WorkT&& work) {
        m_work_queue.push_back(work_type(std::forward<WorkT>(work)));
    }

private:
    boost::sync_queue<work_type> m_work_queue;
    std::vector<boost::scoped_thread<boost::interrupt_and_join_if_joinable>> m_threads;
};

} // namespace executors
} // namespace Melosic

#endif // MELOSIC_EXECUTORS_THREAD_POOL_HPP
