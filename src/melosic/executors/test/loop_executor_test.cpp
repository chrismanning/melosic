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

#include <boost/thread/thread.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/thread/future.hpp>

#include <catch.hpp>

#include <melosic/executors/loop_executor.hpp>

using namespace Melosic::executors;

TEST_CASE("Waiting loop Executor") {
    loop_executor executor;

    SECTION("Basic execution") {
        int i{0};
        executor.submit([&]() { i++; });
        REQUIRE(executor.uninitiated_task_count() == 1);

        REQUIRE(executor.try_run_one_closure());

        CHECK(i == 1);
    }

    SECTION("Serial execution") {
        int i{0};
        executor.submit([&]() { i = 5; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.loop();

        CHECK(i == 6);
        i = 0;

        CHECK(!executor.try_run_one_closure());
        // repeat
        executor.submit([&]() { i = 65; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.loop();

        CHECK(i == 66);
    }

    SECTION("Serial queued execution") {
        int i{0};
        executor.submit([&]() { i = 5; });
        executor.submit([&]() { i++; });

        REQUIRE(executor.uninitiated_task_count() == 2);
        executor.run_queued_closures();

        CHECK(i == 6);
        i = 0;

        CHECK(!executor.try_run_one_closure());
        // repeat
        executor.submit([&]() { i = 65; });
        executor.submit([&]() { i++; });

        REQUIRE(executor.uninitiated_task_count() == 2);
        executor.run_queued_closures();

        CHECK(i == 66);
    }

    SECTION("Serial queued execution", "first make_loop_exit should have no-effect") {
        int i{0};
        executor.submit([&]() { i = 5; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.run_queued_closures();

        CHECK(i == 6);
        i = 0;

        CHECK(!executor.try_run_one_closure());
        // repeat
        executor.submit([&]() { i = 65; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.loop();

        CHECK(i == 66);
    }

    SECTION("Async loop execution") {
        int32_t i{rand()}, scalar{rand()};
        auto fut =
            boost::async(executor, [=]() -> int64_t { return static_cast<int64_t>(i) * static_cast<int64_t>(scalar); });

        REQUIRE(executor.uninitiated_task_count() == 1);
        REQUIRE(executor.try_run_one_closure());
        REQUIRE(fut.is_ready());

        int64_t result{0};
        REQUIRE_NOTHROW(result = fut.get());
        CHECK((static_cast<int64_t>(i) * static_cast<int64_t>(scalar)) == result);
    }

    SECTION("Async loop exception") {
        auto fut = boost::async(executor, []() { throw std::exception(); });

        REQUIRE(executor.uninitiated_task_count() == 1);
        REQUIRE(executor.try_run_one_closure());
        REQUIRE(fut.is_ready());

        CHECK_THROWS_AS(fut.get(), std::exception);
    }
}

TEST_CASE("Threaded waiting loop executor") {
    boost::scoped_thread<> thread;
    loop_executor executor;
    boost::barrier barrier{2};
    auto defaultTimeout = boost::chrono::milliseconds(250);

    thread = boost::scoped_thread<>(&loop_executor::loop, &executor);

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
        auto fut =
            boost::async(executor, [=]() -> int64_t { return static_cast<int64_t>(i) * static_cast<int64_t>(scalar); });

        auto r(fut.wait_for(defaultTimeout));
        REQUIRE(boost::future_status::ready == r);
        int64_t result{0};
        REQUIRE_NOTHROW(result = fut.get());
        CHECK((static_cast<int64_t>(i) * static_cast<int64_t>(scalar)) == result);
    }

    SECTION("Async loop exception") {
        auto fut = boost::async(executor, []() { throw std::exception(); });

        auto r(fut.wait_for(defaultTimeout));
        REQUIRE(boost::future_status::ready == r);
        REQUIRE_THROWS_AS(fut.get(), std::exception);
    }
}

TEST_CASE("Trying loop executor") {
    basic_loop_executor<loop_options::try_yield> executor;

    SECTION("Basic execution") {
        int i{0};
        executor.submit([&]() { i++; });
        REQUIRE(executor.uninitiated_task_count() == 1);

        REQUIRE(executor.try_run_one_closure());

        CHECK(i == 1);
    }

    SECTION("Serial execution") {
        int i{0};
        executor.submit([&]() { i = 5; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.loop();

        CHECK(i == 6);
        i = 0;

        CHECK(!executor.try_run_one_closure());
        // repeat
        executor.submit([&]() { i = 65; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.loop();

        CHECK(i == 66);
    }

    SECTION("Serial queued execution") {
        int i{0};
        executor.submit([&]() { i = 5; });
        executor.submit([&]() { i++; });

        REQUIRE(executor.uninitiated_task_count() == 2);
        executor.run_queued_closures();

        CHECK(i == 6);
        i = 0;

        CHECK(!executor.try_run_one_closure());
        // repeat
        executor.submit([&]() { i = 65; });
        executor.submit([&]() { i++; });

        REQUIRE(executor.uninitiated_task_count() == 2);
        executor.run_queued_closures();

        CHECK(i == 66);
    }

    SECTION("Serial queued execution", "first make_loop_exit should have no-effect") {
        int i{0};
        executor.submit([&]() { i = 5; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.run_queued_closures();

        CHECK(i == 6);
        i = 0;

        CHECK(!executor.try_run_one_closure());
        // repeat
        executor.submit([&]() { i = 65; });
        executor.submit([&]() { i++; });
        executor.submit([&]() { executor.make_loop_exit(); });

        REQUIRE(executor.uninitiated_task_count() == 3);
        executor.loop();

        CHECK(i == 66);
    }

    SECTION("Async loop execution") {
        int32_t i{rand()}, scalar{rand()};
        auto fut =
            boost::async(executor, [=]() -> int64_t { return static_cast<int64_t>(i) * static_cast<int64_t>(scalar); });

        REQUIRE(executor.uninitiated_task_count() == 1);
        REQUIRE(executor.try_run_one_closure());
        REQUIRE(fut.is_ready());

        int64_t result{0};
        REQUIRE_NOTHROW(result = fut.get());
        CHECK((static_cast<int64_t>(i) * static_cast<int64_t>(scalar)) == result);
    }

    SECTION("Async loop exception") {
        auto fut = boost::async(executor, []() { throw std::exception(); });

        REQUIRE(executor.uninitiated_task_count() == 1);
        REQUIRE(executor.try_run_one_closure());
        REQUIRE(fut.is_ready());

        CHECK_THROWS_AS(fut.get(), std::exception);
    }
}

TEST_CASE("Threaded trying loop executor") {
    boost::scoped_thread<> thread;
    basic_loop_executor<loop_options::try_yield> executor;
    boost::barrier barrier{2};
    auto defaultTimeout = boost::chrono::milliseconds(250);

    thread = boost::scoped_thread<>(&basic_loop_executor<loop_options::try_yield>::loop, &executor);

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
        auto fut =
            boost::async(executor, [=]() -> int64_t { return static_cast<int64_t>(i) * static_cast<int64_t>(scalar); });

        auto r(fut.wait_for(defaultTimeout));
        REQUIRE(boost::future_status::ready == r);
        int64_t result{0};
        REQUIRE_NOTHROW(result = fut.get());
        CHECK((static_cast<int64_t>(i) * static_cast<int64_t>(scalar)) == result);
    }

    SECTION("Async loop exception") {
        auto fut = boost::async(executor, []() { throw std::exception(); });

        auto r(fut.wait_for(defaultTimeout));
        REQUIRE(boost::future_status::ready == r);
        REQUIRE_THROWS_AS(fut.get(), std::exception);
    }
}
