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

#include <melosic/common/string.hpp>

static const std::string testString1("The Quick Brown Fox Jumps Over The Lazy Dog");

TEST(StringTest, ToLowerCaseTest) {
    EXPECT_EQ("the quick brown fox jumps over the lazy dog", toLower(testString1));
}

TEST(StringTest, ToTitleCaseTest) {
    EXPECT_EQ(testString1, toTitle(toUpper(testString1)));
    EXPECT_EQ(testString1, toTitle(toLower(testString1)));
}
