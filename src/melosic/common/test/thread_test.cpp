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

#include <chrono>
#include <cstdlib>

#include <gtest/gtest.h>

#include <melosic/common/thread.hpp>

struct ThreadTest : ::testing::Test {
protected:
    Melosic::Thread::Manager tman;
    std::chrono::milliseconds defaultTimeout{500};
};

TEST_F(ThreadTest, ThreadPoolEnqueue) {
    int32_t i{rand()}, scalar{rand()};
    auto fut = tman.enqueue([=] () -> int64_t {
        return static_cast<int64_t>(i) * static_cast<int64_t>(scalar);
    });

    auto r(fut.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Task deferred or timed-out";
    int64_t result{0};
    ASSERT_NO_THROW(result = fut.get()) << "Task should not throw anything";
    EXPECT_EQ(static_cast<int64_t>(i) * static_cast<int64_t>(scalar), result) << "Task failed";
}

TEST_F(ThreadTest, ThreadPoolEnqueueException) {
    auto fut = tman.enqueue([] () {
        throw std::exception();
    });

    auto r(fut.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Task deferred or timed-out";
    ASSERT_THROW(fut.get(), std::exception) << "Task should throw an exception and copy it through the promise";
}
