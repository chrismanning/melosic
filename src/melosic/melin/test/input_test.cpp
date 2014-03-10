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

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/utility/string_ref.hpp>
using namespace boost::literals;

#include <network/uri.hpp>

#include <melosic/melin/input.hpp>
using namespace Melosic::Input;

struct InputTest : ::testing::Test {
protected:
    Manager inman;
    std::chrono::milliseconds defaultTimeout{500};
};

TEST_F(InputTest, InputManagerTest1) {

}

TEST(InputMiscTest, PathToUrlTest1) {
    fs::path p;
    ASSERT_NO_THROW(p = "/some/file path/with spaces");
    network::uri uri;
    ASSERT_NO_THROW(uri = ::to_uri(p));

    EXPECT_EQ("file:///some/file%20path/with%20spaces", uri.to_string<char>());
}

TEST(InputMiscTest, PathToUrlTest2) {
    fs::path p;
    ASSERT_NO_THROW(p = "/some/file path/with (parens) abc.ext");
    network::uri uri;
    ASSERT_NO_THROW(uri = ::to_uri(p));

    EXPECT_EQ("file:///some/file%20path/with%20%28parens%29%20abc.ext", uri.to_string<char>());
}

TEST(InputMiscTest, UrlToPathTest1) {
    network::uri uri;
    ASSERT_NO_THROW(uri = network::uri("file:///some/file%20path/with%20spaces"));
    fs::path p;
    ASSERT_NO_THROW(p = uri_to_path(uri));

    EXPECT_EQ("/some/file path/with spaces", p.string());
}

TEST(InputMiscTest, UrlToPathTest2) {
    network::uri uri;
    ASSERT_NO_THROW(uri = network::uri("file:///some/file%20path/with%20%28parens%29%20abc.ext"));
    fs::path p;
    ASSERT_NO_THROW(p = uri_to_path(uri));

    EXPECT_EQ("/some/file path/with (parens) abc.ext", p.string());
}
