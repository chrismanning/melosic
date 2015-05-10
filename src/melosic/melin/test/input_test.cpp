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

#include <catch.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <chrono>
using namespace std::literals;

#include <network/uri.hpp>

#include <melosic/melin/input.hpp>
using namespace Melosic::Input;

TEST_CASE("InputTest") {
    Manager inman;
    auto defaultTimeout = 500ms;
}

TEST_CASE("PathToUrlTest1") {
    fs::path p;
    REQUIRE_NOTHROW(p = "/some/file path/with spaces");
    network::uri uri;
    REQUIRE_NOTHROW(uri = ::to_uri(p));

    CHECK("file:///some/file%20path/with%20spaces" == uri.to_string<char>());
}

TEST_CASE("PathToUrlTest2") {
    fs::path p;
    REQUIRE_NOTHROW(p = "/some/file path/with (parens) abc.ext");
    network::uri uri;
    REQUIRE_NOTHROW(uri = ::to_uri(p));

    CHECK("file:///some/file%20path/with%20%28parens%29%20abc.ext" == uri.to_string<char>());
}

TEST_CASE("PathToUrlTest3") {
    fs::path p;
    REQUIRE_NOTHROW(p = "/some/file path/with [brackets] abc.ext");
    network::uri uri;
    REQUIRE_NOTHROW(uri = ::to_uri(p));

    CHECK("file:///some/file%20path/with%20%5Bbrackets%5D%20abc.ext" == uri.to_string<char>());
}

TEST_CASE("PathToUrlTest4") {
    fs::path p;
    REQUIRE_NOTHROW(p = "/some/file path/with ütf8.ext");
    network::uri uri;
    REQUIRE_NOTHROW(uri = ::to_uri(p));

    CHECK("file:///some/file%20path/with%20%C3%BCtf8.ext" == uri.to_string<char>());
}

TEST_CASE("PathToUrlTest5") {
    fs::path p;
    REQUIRE_NOTHROW(p = "/some/file path/with \t tab.ext");
    network::uri uri;
    REQUIRE_NOTHROW(uri = ::to_uri(p));

    CHECK("file:///some/file%20path/with%20%09%20tab.ext" == uri.to_string<char>());
}

TEST_CASE("UrlToPathTest1") {
    network::uri uri;
    REQUIRE_NOTHROW(uri = network::uri("file:///some/file%20path/with%20spaces"));
    fs::path p;
    REQUIRE_NOTHROW(p = uri_to_path(uri));

    CHECK("/some/file path/with spaces" == p.string());
}

TEST_CASE("UrlToPathTest2") {
    network::uri uri;
    REQUIRE_NOTHROW(uri = network::uri("file:///some/file%20path/with%20%28parens%29%20abc.ext"));
    fs::path p;
    REQUIRE_NOTHROW(p = uri_to_path(uri));

    CHECK("/some/file path/with (parens) abc.ext" == p.string());
}
