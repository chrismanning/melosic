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

#include <string>
using namespace std::literals;
#include <list>

#include <boost/range/algorithm/equal.hpp>

#include <gtest/gtest.h>

#include <melosic/common/threadsafe_list.hpp>
using namespace Melosic;

struct ListTest : ::testing::Test {

    struct ThrowCopy {
        ThrowCopy() = default;
        ThrowCopy(const ThrowCopy&) {
            throw std::exception();
        }
        ThrowCopy(ThrowCopy&&) = default;
    };

    threadsafe_list<int> int_list;
    threadsafe_list<ThrowCopy> thrower_list;
    threadsafe_list<std::string> string_list;
};

TEST_F(ListTest, PushFront) {
    ASSERT_EQ(0u, int_list.size());
    int_list.push_front(123);
    EXPECT_EQ(1u, int_list.size());
    int_list.push_front(321);
    int_list.push_front(4566578);
    EXPECT_EQ(3u, int_list.size());

    ThrowCopy tmp;
    ASSERT_THROW(thrower_list.push_front(tmp), std::exception);
    EXPECT_EQ(0u, thrower_list.size());
    ASSERT_NO_THROW(thrower_list.push_front(ThrowCopy()));
    EXPECT_EQ(1u, thrower_list.size());
    ASSERT_THROW(thrower_list.push_front(tmp), std::exception);
    EXPECT_EQ(1u, thrower_list.size());
}

TEST_F(ListTest, PushBack) {
    ASSERT_EQ(0u, int_list.size());
    int_list.push_back(123);
    EXPECT_EQ(1u, int_list.size());
    int_list.push_back(321);
    int_list.push_back(4566578);
    EXPECT_EQ(3u, int_list.size());

    ThrowCopy tmp;
    ASSERT_THROW(thrower_list.push_back(tmp), std::exception);
    EXPECT_EQ(0u, thrower_list.size());
    ASSERT_NO_THROW(thrower_list.push_back(ThrowCopy()));
    EXPECT_EQ(1u, thrower_list.size());
    ASSERT_THROW(thrower_list.push_back(tmp), std::exception);
    EXPECT_EQ(1u, thrower_list.size());
}

TEST_F(ListTest, Find) {
    int_list.push_front(123);
    int_list.push_front(321);
    const int x = 4566578;
    int_list.push_front(x);
    ASSERT_EQ(3u, int_list.size());

    {
        auto ptr = int_list.find_first_if([] (int i) { return !(i % 2); });
        EXPECT_EQ(x, *ptr);

        ptr = int_list.find_first_if([] (int) { return false; });
        EXPECT_EQ(nullptr, ptr);
    }

    {
        const auto& const_ref = int_list;
        auto ptr = const_ref.find_first_if([] (int i) { return !(i % 2); });
        EXPECT_EQ(x, *ptr);
    }

    std::string str1("abc43"s);
    string_list.push_front(str1);
    std::string str2("defdfg134"s);
    string_list.push_front(str2);
    ASSERT_EQ(2u, string_list.size());

    {
        auto ptr = string_list.find_first_if([] (const std::string& str) {
            return str.size() == 5;
        });
        EXPECT_EQ(str1, *ptr);

        ptr = string_list.find_first_if([] (const std::string&) { return false; });
        EXPECT_EQ(nullptr, ptr);
    }

    {
        const auto& const_ref = string_list;
        auto ptr = const_ref.find_first_if([] (const std::string& str) {
            return str[0] == 'd';
        });
        EXPECT_EQ(str2, *ptr);
    }
}

TEST_F(ListTest, ForEach) {
    const int a = 123, b = 321, c = 4566578;
    int_list.push_front(a);
    int_list.push_front(b);
    int_list.push_front(c);
    ASSERT_EQ(3u, int_list.size());

    {
        const auto& const_ref = int_list;
        std::list<int> std_int_list;
        const_ref.for_each([&] (const int& val) {
            std_int_list.push_front(val);
        });
        EXPECT_EQ(std_int_list, std::list<int>({a,b,c}));
    }

    {
        std::list<int> std_int_list;
        int_list.for_each([&] (int& val) {
            ++val;
        });
        int_list.for_each([&] (const int& val) {
            std_int_list.push_front(val);
        });
        EXPECT_EQ(std_int_list, std::list<int>({a+1,b+1,c+1}));
    }
}

TEST_F(ListTest, Stream) {
    const int a = 123, b = 321, c = 4566578;
    int_list.push_front(a);
    int_list.push_front(b);
    int_list.push_front(c);
    std::stringstream str;
    str << int_list;
    EXPECT_EQ("[4566578,321,123]", str.str());
}

TEST_F(ListTest, Stream2) {
    const int a = 123, b = 321, c = 4566578;
    int_list.push_back(a);
    int_list.push_back(b);
    int_list.push_back(c);
    std::stringstream str;
    str << int_list;
    EXPECT_EQ("[123,321,4566578]", str.str());
}

TEST_F(ListTest, Equality) {
    const int a = 123, b = 321, c = 4566578;
    int_list.push_front(a);
    int_list.push_front(b);
    int_list.push_front(c);

    decltype(int_list) b_list;
    b_list.push_front(a);
    b_list.push_front(b);
    b_list.push_front(c);

    EXPECT_EQ(b_list, int_list);
}

TEST_F(ListTest, CopyConstructor) {
    const int a = 123, b = 321, c = 4566578;
    int_list.push_front(a);
    int_list.push_front(b);
    int_list.push_front(c);

    decltype(int_list) b_list(int_list);
    EXPECT_EQ(int_list, b_list);
}

TEST_F(ListTest, RemoveIf) {
    const int a = 123, b = 321, c = 4566578;
    int_list.push_front(a);
    int_list.push_front(b);
    int_list.push_front(c);

    int_list.remove_if([=] (int i) { return i == b; });
    EXPECT_EQ(2u, int_list.size());
    decltype(int_list) b_list;
    b_list.push_front(a);
    b_list.push_front(c);
    EXPECT_EQ(b_list, int_list);

    int_list.clear();
    ASSERT_EQ(0u, int_list.size());

    int_list.push_front(a);
    int_list.push_front(b);
    int_list.push_front(c);
    EXPECT_EQ(3u, int_list.size());

    int_list.remove_if([] (int i) -> bool { return i % 2; });
    EXPECT_EQ(1u, int_list.size());

    auto x = int_list.find_first_if([] (int) { return true; });
    EXPECT_EQ(c, *x);
}
