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

#include <catch.hpp>

#include <future>

#include <boost/thread/barrier.hpp>
#include <boost/thread/future.hpp>

#include <melosic/executors/thread_pool.hpp>
using namespace Melosic::executors;

TEST_CASE("Thread pool executor") {
    boost::barrier barrier{2};
    thread_pool executor;
    auto defaultTimeout = boost::chrono::milliseconds(250);

    SECTION("Basic thread execution") {
        int i{0};
        executor.submit([&]() {
            i++;
            barrier.wait();
        });
        barrier.wait();

        CHECK(i == 1);
    }

    SECTION("Basic thread execution", "delayed wait") {
        int i{0};
        executor.submit([&]() {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            i++;
            barrier.wait();
        });
        barrier.wait();

        CHECK(i == 1);
    }

    SECTION("Basic thread execution", "delayed") {
        int i{0};
        executor.submit([&]() {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            i++;
        });
    }

    SECTION("Async loop execution") {
        int32_t i{rand()}, scalar{rand()};
        auto fut = boost::async(executor, [=] () -> int64_t {
            return static_cast<int64_t>(i) * static_cast<int64_t>(scalar);
        });

        auto r(fut.wait_for(defaultTimeout));
        REQUIRE(boost::future_status::ready == r);
        int64_t result{0};
        REQUIRE_NOTHROW(result = fut.get());
        CHECK((static_cast<int64_t>(i) * static_cast<int64_t>(scalar)) == result);
    }

    SECTION("Async loop exception") {
        auto fut = boost::async(executor, [] () {
            throw std::exception();
        });

        auto r(fut.wait_for(defaultTimeout));
        REQUIRE(boost::future_status::ready == r);
        REQUIRE_THROWS_AS(fut.get(), std::exception);
    }

    SECTION("Packaged task execution") {
        auto i = rand();
        std::packaged_task<bool()> task([i]() {
            return i % 2;
        });
        auto fut = task.get_future();
        executor.submit(std::move(task));

        auto r(fut.wait_for(std::chrono::milliseconds(250)));
        REQUIRE(std::future_status::ready == r);
        REQUIRE_NOTHROW(CHECK((i%2) == fut.get()));
    }
}
