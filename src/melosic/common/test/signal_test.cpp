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

#include <gtest/gtest.h>

#include <melosic/common/signal.hpp>

typedef Melosic::Signals::Signal<void(int)> SignalType;

struct SignalTest : ::testing::Test {
protected:
    SignalType s1;
    std::mutex mu;
    std::chrono::milliseconds defaultTimeout{500};
};

TEST_F(SignalTest, SignalConnectTest) {
    s1.connect([] (int) {});
    ASSERT_EQ(s1.slotCount(), 1) << "Slot not added";
}

TEST_F(SignalTest, SignalCallTest) {
    int a = 5, b = 12;
    s1.connect([&] (int s) { a *= s; });
    s1.connect([&] (int s) { b *= s; });
    auto scalar(3);
    auto f(s1(scalar));
    auto r = f.wait_for(defaultTimeout);
    ASSERT_EQ(r, std::future_status::ready) << "Signal call deferred or timed-out";
    ASSERT_EQ(a, 5*scalar) << "First slot not called";
    ASSERT_EQ(b, 12*scalar) << "Second slot not called";
}
