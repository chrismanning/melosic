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

#include <catch.hpp>

#include <melosic/common/thread.hpp>

TEST_CASE("Thread test") {
    Melosic::Thread::Manager tman;
    auto defaultTimeout = 500ms;

SECTION("ThreadPoolEnqueue") {
    int32_t i{rand()}, scalar{rand()};
    auto fut = tman.enqueue([=] () -> int64_t {
        return static_cast<int64_t>(i) * static_cast<int64_t>(scalar);
    });

    auto r(fut.wait_for(defaultTimeout));
    REQUIRE(std::future_status::ready == r);
    int64_t result{0};
    REQUIRE_NOTHROW(result = fut.get());
    CHECK((static_cast<int64_t>(i) * static_cast<int64_t>(scalar)) == result);
}

SECTION("ThreadPoolEnqueueException") {
    auto fut = tman.enqueue([] () {
        throw std::exception();
    });

    auto r(fut.wait_for(defaultTimeout));
    REQUIRE(std::future_status::ready == r);
    REQUIRE_THROWS_AS(fut.get(), std::exception);
}

}
