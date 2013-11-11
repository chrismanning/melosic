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

#include <memory>
#include <cstdlib>

#include <boost/smart_ptr/make_shared.hpp>

#include <gtest/gtest.h>

#include <melosic/common/signal.hpp>

typedef Melosic::Signals::Signal<void(int32_t)> SignalType;

struct SignalTest : ::testing::Test {
protected:
    SignalType sig1;
    std::chrono::milliseconds defaultTimeout{500};
};

TEST_F(SignalTest, SignalConnectTest) {
    ASSERT_EQ(0u, sig1.slotCount()) << "Should not be any slots connected";
    auto c = sig1.connect([] (int32_t) {});
    EXPECT_EQ(1u, sig1.slotCount()) << "Slot not added";
    c.disconnect();
    EXPECT_EQ(0u, sig1.slotCount()) << "Slot wasn't disconnected";
    Melosic::Signals::ScopedConnection sc = sig1.connect([] (int32_t) {});
    EXPECT_EQ(1u, sig1.slotCount()) << "Slot not added";
    sc = Melosic::Signals::ScopedConnection{};
    EXPECT_EQ(0u, sig1.slotCount()) << "Slot wasn't disconnected";
}

TEST_F(SignalTest, SignalCallTest) {
    ASSERT_EQ(0u, sig1.slotCount()) << "Should not be any slots connected";
    int64_t a{rand()}, b{rand()};
    const int64_t a_{a}, b_{b};
    sig1.connect([&] (int32_t s) { a *= s; });
    sig1.connect([&] (int32_t s) { b *= s; });
    int scalar{rand()};
    auto f(sig1(scalar));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(a_*scalar, a) << "First slot call failed";
    EXPECT_EQ(b_*scalar, b) << "Second slot call failed";
}

struct S {
    void fun(int32_t a) {
        m_a = a;
    }
    int32_t m_a{0};
};

TEST_F(SignalTest, SignalBindTest) {
    ASSERT_EQ(0u, sig1.slotCount()) << "Should not be any slots connected";
    S s;
    auto c = sig1.connect(&S::fun, &s);
    EXPECT_EQ(1u, sig1.slotCount()) << "Slot not added";
    int32_t i{rand()};
    auto f(sig1(i));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(i, s.m_a) << "Slot call failed";
    c.disconnect();
    EXPECT_EQ(0u, sig1.slotCount()) << "Slot wasn't disconnected";
}

TEST_F(SignalTest, SignalSharedBindTest) {
    ASSERT_EQ(0u, sig1.slotCount()) << "Should not be any slots connected";
    auto s(std::make_shared<S>());

    auto c = sig1.connect(&S::fun, s);

    int32_t i{rand()};
    auto f(sig1(i));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(i, s->m_a) << "Slot call failed";

    s.reset();
    i = rand();
    f = sig1(i);
    r = f.wait_for(defaultTimeout);
    EXPECT_NO_THROW(f.get()) << "std::function threw";
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(0u, sig1.slotCount()) << "Slot wasn't disconnected when expired";
}

TEST_F(SignalTest, BoostSignalSharedBindTest) {
    ASSERT_EQ(0u, sig1.slotCount()) << "Should not be any slots connected";
    auto s(boost::make_shared<S>());

    auto c = sig1.connect(&S::fun, s);

    int32_t i{rand()};
    auto f(sig1(i));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(i, s->m_a) << "Slot call failed";

    s.reset();
    i = rand();
    f = sig1(i);
    r = f.wait_for(defaultTimeout);
    EXPECT_NO_THROW(f.get()) << "std::function threw";
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(0u, sig1.slotCount()) << "Slot wasn't disconnected when expired";
}

TEST_F(SignalTest, ObjSignalBindTest) {
    ASSERT_EQ(0u, sig1.slotCount()) << "Should not be any slots connected";
    S s;
    auto c = sig1.connect(&S::fun, s);
    EXPECT_EQ(1u, sig1.slotCount()) << "Slot not added";
    int32_t i{rand()};
    auto f(sig1(i));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(i, s.m_a) << "Slot call failed";
    c.disconnect();
    EXPECT_EQ(0u, sig1.slotCount()) << "Slot wasn't disconnected";
}
