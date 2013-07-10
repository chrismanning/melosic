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

#include "thread.hpp"

namespace Melosic {
namespace Thread {

Manager::Manager(const unsigned n) :
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
    LOG(logject) << threads.size() << " threads started in thread pool";
}

Manager::~Manager() {
    done = true;
    for(auto& t : threads)
        t.join();
}

bool Manager::contains(std::thread::id id) const {
    for(auto&& t : threads)
        if(t.get_id() == id)
            return true;

    return false;
}

} // namespace Thread
} // namespace Melosic
