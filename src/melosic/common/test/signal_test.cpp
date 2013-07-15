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

#include <gtest/gtest.h>

#include <melosic/common/ptr_adaptor.hpp>
#include <melosic/common/signal.hpp>

typedef Melosic::Signals::Signal<void(int)> SignalType;

struct SignalTest : ::testing::Test {
protected:
    SignalType sig1;
    std::chrono::milliseconds defaultTimeout{500};
};

TEST_F(SignalTest, SignalConnectTest) {
    ASSERT_EQ(0, sig1.slotCount()) << "Should not be any slots connected";
    auto c = sig1.connect([] (int) {});
    EXPECT_EQ(1, sig1.slotCount()) << "Slot not added";
    c.disconnect();
    EXPECT_EQ(0, sig1.slotCount()) << "Slot wasn't disconnected";
}

TEST_F(SignalTest, SignalCallTest) {
    ASSERT_EQ(0, sig1.slotCount()) << "Should not be any slots connected";
    int a{5}, b{12};
    const int a_{a}, b_{b};
    sig1.connect([&] (int s) { a *= s; });
    sig1.connect([&] (int s) { b *= s; });
    auto scalar(3);
    auto f(sig1(scalar));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(a_*scalar, a) << "First slot call failed";
    EXPECT_EQ(b_*scalar, b) << "Second slot call failed";
}

struct S {
    void fun(int a) {
        m_a = a;
    }
    int m_a{0};
};

TEST_F(SignalTest, SignalBindTest) {
    ASSERT_EQ(0, sig1.slotCount()) << "Should not be any slots connected";
    S s;
    auto c = sig1.connect(&S::fun, &s, ph::_1);
    EXPECT_EQ(1, sig1.slotCount()) << "Slot not added";
    int i{37};
    auto f(sig1(i));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(i, s.m_a) << "Slot call failed";
    c.disconnect();
    EXPECT_EQ(0, sig1.slotCount()) << "Slot wasn't disconnected";
}

TEST_F(SignalTest, WeakPtrAdaptorTest) {
    auto s(std::make_shared<S>());
    auto fun = Melosic::bindWeakPtr(s);
    EXPECT_NO_THROW(fun()) << "Ptr should be good";

    s.reset();
    EXPECT_THROW(fun(), std::bad_weak_ptr) << "Ptr should be bad";
}

TEST_F(SignalTest, WeakPtrAdaptorBindTest) {
    auto s(std::make_shared<S>());

    auto mf = std::bind(&S::fun, Melosic::bindWeakPtr(s), ph::_1);
    EXPECT_NO_THROW(mf(2)) << "Ptr should be good";
    s.reset();
    EXPECT_THROW(mf(2), std::bad_weak_ptr) << "Ptr should be bad";
}

TEST_F(SignalTest, SignalSharedBindTest) {
    ASSERT_EQ(0, sig1.slotCount()) << "Should not be any slots connected";
    auto s(std::make_shared<S>());

    auto c = sig1.connect(&S::fun, s, ph::_1);

    int i{37};
    auto f(sig1(i));
    auto r(f.wait_for(defaultTimeout));
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(i, s->m_a) << "Slot call failed";

    s.reset();
    i = 81;
    f = sig1(i);
    r = f.wait_for(defaultTimeout);
    EXPECT_NO_THROW(f.get()) << "std::function threw";
    ASSERT_EQ(std::future_status::ready, r) << "Signal call deferred or timed-out";
    EXPECT_EQ(0, sig1.slotCount()) << "Slot wasn't disconnected when expired";
}
