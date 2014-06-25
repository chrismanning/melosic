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
#include <boost/chrono.hpp>

#include <catch.hpp>

#include <melosic/common/signal.hpp>

typedef Melosic::Signals::Signal<void(int32_t)> SignalType;
auto use_future = Melosic::Signals::use_future;

static void sleep_slot(int32_t) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
}

TEST_CASE("Signal Test") {
    SignalType sig1;
    const auto defaultTimeout = boost::chrono::milliseconds(500);

    REQUIRE(0u == sig1.slotCount());

SECTION("SignalConnectTest") {
    auto c = sig1.connect([] (int32_t) {});
    CHECK(1u == sig1.slotCount());
    c.disconnect();
    CHECK(0u == sig1.slotCount());
    Melosic::Signals::ScopedConnection sc = sig1.connect([] (int32_t) {});
    CHECK(1u == sig1.slotCount());
    sc = Melosic::Signals::ScopedConnection{};
    CHECK(0u == sig1.slotCount());
}

SECTION("SignalDisconnectTest") {
    Melosic::Signals::Connection c;
    {
        SignalType sig2;
        c = sig2.connect([] (int32_t) {});
        CHECK(1u == sig2.slotCount());
    }
    CHECK(!c.isConnected());
}

SECTION("SignalCallTest") {
    int64_t a{rand()}, b{rand()};
    const int64_t a_{a}, b_{b};
    sig1.connect([&] (int32_t s) { a *= s; });
    sig1.connect([&] (int32_t s) { b *= s; });
    int scalar{rand()};
    auto f(sig1(use_future, scalar));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK((a_*scalar) == a);
    CHECK((b_*scalar) == b);
}

typedef Melosic::Signals::Signal<void(int32_t&)> IntRefSignalType;
SECTION("SignalWrappedReferenceTest") {
    IntRefSignalType ref_sig;

    int32_t a{rand()};
    const int32_t a_{a};
    ref_sig.connect([&a](int32_t& s) {
        s++;
        CHECK(&a == &s);
    });

    auto f(ref_sig(use_future, std::ref(a)));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK((a_+1) == a);
}

SECTION("SignalReferenceTest") {
    IntRefSignalType ref_sig;

    int32_t a{rand()};
    const int32_t a_{a};
    ref_sig.connect([&a](int32_t& s) {
        s++;
        CHECK(&a == &s);
    });

    auto f(ref_sig(use_future, a));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK((a_+1) == a);
}

struct Abc {
    Abc() = default;
    Abc(const Abc&) {
        std::cout << "COPYING\n";
    }
    Abc(Abc&&) {
        std::cout << "MOVING\n";
    }
};

typedef Melosic::Signals::Signal<void(Abc&)> AbcRefSignalType;
SECTION("SignalWrappedStructReferenceTest") {
    AbcRefSignalType ref_sig;

    Abc a;
    ref_sig.connect([&a](Abc& s) { CHECK(&s == &a); });

    auto f(ref_sig(use_future, std::ref(a)));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
}

SECTION("SignalStructReferenceTest") {
    AbcRefSignalType ref_sig;

    Abc a;
    ref_sig.connect([&a](Abc& s) { CHECK(&s == &a); });

    auto f(ref_sig(use_future, a));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
}

struct S {
    void fun(int32_t a) {
        m_a = a;
    }
    int32_t m_a{0};
};

SECTION("SignalBindTest") {
    S s;
    auto c = sig1.connect(&S::fun, &s);
    CHECK(1u == sig1.slotCount());
    int32_t i{rand()};
    auto f(sig1(use_future, i));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK(i == s.m_a);
    c.disconnect();
    CHECK(0u == sig1.slotCount());
}

SECTION("SignalSharedBindTest") {
    auto s(std::make_shared<S>());

    auto c = sig1.connect(&S::fun, s);

    int32_t i{rand()};
    auto f(sig1(use_future, i));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK(i == s->m_a);

    s.reset();
    i = rand();
    f = sig1(use_future, i);
    r = f.wait_for(defaultTimeout);
    CHECK_NOTHROW(f.get());
    REQUIRE(boost::future_status::ready == r);
    CHECK(0u == sig1.slotCount());
}

SECTION("BoostSignalSharedBindTest") {
    auto s(boost::make_shared<S>());

    auto c = sig1.connect(&S::fun, s);

    int32_t i{rand()};
    auto f(sig1(use_future, i));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK(i == s->m_a);

    s.reset();
    i = rand();
    f = sig1(use_future, i);
    r = f.wait_for(defaultTimeout);
    CHECK_NOTHROW(f.get());
    REQUIRE(boost::future_status::ready == r);
    CHECK(0u == sig1.slotCount());
}

SECTION("ObjSignalBindTest") {
    S s;
    auto c = sig1.connect(&S::fun, s);
    CHECK(1u == sig1.slotCount());
    int32_t i{rand()};
    auto f(sig1(use_future, i));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK(i == s.m_a);
    c.disconnect();
    CHECK(0u == sig1.slotCount());
}

typedef Melosic::Signals::Signal<void(int32_t)> NestedSignalType;
SECTION("NestedSignalTest") {
    NestedSignalType sig2;
    int32_t i{rand()};
    int32_t res;
    auto c1 = sig1.connect([&](int32_t a) { sig2(use_future, a).wait(); });
    auto c2 = sig2.connect([&](int32_t a) { res = a+1; });
    CHECK(1u == sig1.slotCount());
    CHECK(1u == sig2.slotCount());

    auto f(sig1(use_future, i));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);

    CHECK(res == i+1);
    c1.disconnect();
    c2.disconnect();
    CHECK(0u == sig1.slotCount());
    CHECK(0u == sig2.slotCount());
}

SECTION("NestedSignalDisconnectTest") {
    NestedSignalType sig2;
    int32_t i{rand()};
    auto c1 = sig1.connect([&](int32_t a) { sig2(use_future, a).wait(); });
    auto c2 = sig2.connect([&](int32_t) { throw 0; });
    CHECK(1u == sig1.slotCount());
    CHECK(1u == sig2.slotCount());

    auto f(sig1(use_future, i));
    auto r(f.wait_for(defaultTimeout));
    REQUIRE(boost::future_status::ready == r);
    CHECK(0u == sig2.slotCount());

    c1.disconnect();
    c2.disconnect();
    CHECK(0u == sig1.slotCount());
    CHECK(0u == sig2.slotCount());
}

SECTION("BlockingSignalTest") {
    sig1.connect(sleep_slot);
    sig1(0);
}

SECTION("BlockingFutureSignalTest") {
    sig1.connect(sleep_slot);
    sig1(use_future, 0);
}

}
