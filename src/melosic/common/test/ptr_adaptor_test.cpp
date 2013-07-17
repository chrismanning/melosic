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
#include <functional>
namespace ph = std::placeholders;
#include <cstdlib>

#include <boost/smart_ptr.hpp>

#include <gtest/gtest.h>

#include <melosic/common/ptr_adaptor.hpp>

struct S {
    void fun(int32_t a) {
        m_a = a;
    }
    int32_t m_a{0};
};

TEST(PtrBindTest, BoostWeakPtrAdaptorTest) {
    auto s(boost::make_shared<S>());
    auto fun = Melosic::bindObj(s);
    EXPECT_NO_THROW(fun()) << "Ptr should be good";

    s.reset();
    EXPECT_THROW(fun(), std::bad_weak_ptr) << "Ptr should be bad";
}

TEST(PtrBindTest, BoostWeakPtrAdaptorBindTest) {
    auto s(boost::make_shared<S>());

    auto mf = std::bind(&S::fun, Melosic::bindObj(s), ph::_1);
    EXPECT_NO_THROW(mf(std::rand())) << "Ptr should be good";
    s.reset();
    EXPECT_THROW(mf(std::rand()), std::bad_weak_ptr) << "Ptr should be bad";
}

TEST(PtrBindTest, WeakPtrAdaptorTest) {
    auto s(std::make_shared<S>());
    auto fun = Melosic::bindObj(s);
    EXPECT_NO_THROW(fun()) << "Ptr should be good";

    s.reset();
    EXPECT_THROW(fun(), std::bad_weak_ptr) << "Ptr should be bad";
}

TEST(PtrBindTest, WeakPtrAdaptorBindTest) {
    auto s(std::make_shared<S>());

    auto mf = std::bind(&S::fun, Melosic::bindObj(s), ph::_1);
    EXPECT_NO_THROW(mf(std::rand())) << "Ptr should be good";
    s.reset();
    EXPECT_THROW(mf(std::rand()), std::bad_weak_ptr) << "Ptr should be bad";
}

TEST(PtrBindTest, ObjBindTest) {
    S s;

    auto mf = std::bind(&S::fun, Melosic::bindObj(s), ph::_1);
    int32_t i{std::rand()};
    mf(i);
    EXPECT_EQ(i, s.m_a) << "Bind call failed";
}
