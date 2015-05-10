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

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/mpl/index_of.hpp>
#include <boost/variant.hpp>

#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/common/signal_core.hpp>
using namespace Melosic;

template <typename T> static constexpr int TypeIndex = boost::mpl::index_of<Config::VarType::types, T>::type::value;

TEST_CASE("ConfVars") {
    Config::VarType var = "The quick brown fox jumped over the lazy dog."s;
    CHECK(TypeIndex<std::string> == var.which());
    var = true;
    CHECK(TypeIndex<bool> == var.which());
    var = 125U;
    CHECK(TypeIndex<uint32_t> == var.which());
    var = -125;
    CHECK(TypeIndex<int32_t> == var.which());
    var = 125UL;
    CHECK(TypeIndex<uint64_t> == var.which());
    var = -125L;
    CHECK(TypeIndex<int64_t> == var.which());
    var = 6573.738504;
    CHECK(TypeIndex<double> == var.which());
}

TEST_CASE("ConfNodes") {
    const std::string name("Test Conf Tree");
    Config::Conf c(name);
    CHECK(name == c.name());
    CHECK(0u == c.nodeCount());

    {
        std::string str_node("adf");
        c.putNode("str_key", str_node);
        CHECK(1u == c.nodeCount());
        auto node = c.getNode("str_key");
        REQUIRE(node);
        CHECK(TypeIndex<std::string> == node->which());
        CHECK(str_node == boost::get<std::string>(*node));
    }

    {
        c.putNode("str_key", 24564235465UL);
        auto node = c.getNode("str_key");
        REQUIRE(node);
        REQUIRE(TypeIndex<uint64_t> == node->which());
        CHECK(24564235465UL == boost::get<uint64_t>(*node));
    }

    c.removeNode("str_key");
    CHECK(0u == c.nodeCount());
}

TEST_CASE("ConfChild") {
    Config::Conf base("root");
    const std::string name("Test Conf Tree");
    Config::Conf tmp(name);
    base.putChild(std::move(tmp));
    CHECK(0u == base.nodeCount());
    CHECK(1u == base.childCount());

    auto child = base.getChild(name);
    REQUIRE(child);
    CHECK(name == child->name());

    CHECK(child.use_count() == 2);
    base.removeChild(name);
    CHECK(child.use_count() == 1);
    CHECK(0u == base.childCount());
}

TEST_CASE("ConfCopy") {
    Config::Conf c1("root sdf asdr");
    c1.putNode("test", 1066);
    CHECK(1u == c1.nodeCount());
    Config::Conf c2(c1);
    CHECK(1u == c2.nodeCount());

    {
        auto node = c1.getNode("test");
        REQUIRE(node);
        CHECK(TypeIndex<int32_t> == node->which());
        CHECK(1066 == boost::get<int32_t>(*node));
    }

    c1.removeNode("test");
    CHECK(0u == c1.nodeCount());
    CHECK(1u == c2.nodeCount());
}

TEST_CASE("ConfMerge") {
    Config::Conf c1("root");
    c1.putNode("PI", 3.1415926);
    c1.putNode("PI_Str", "3.1415926"s);
    CHECK(2u == c1.nodeCount());

    Config::Conf c2("root2");
    c2.putNode("PI_Int", 3);

    c1.merge(c2);
    CHECK(1u == c2.nodeCount());
    CHECK(3u == c1.nodeCount());

    {
        auto node = c1.getNode("PI");
        REQUIRE(node);
        CHECK(TypeIndex<double> == node->which());
        CHECK(3.1415926 == boost::get<double>(*node));
    }

    {
        auto node = c1.getNode("PI_Str");
        REQUIRE(node);
        CHECK(TypeIndex<std::string> == node->which());
        CHECK("3.1415926" == boost::get<std::string>(*node));
    }

    {
        auto node = c1.getNode("PI_Int");
        REQUIRE(node);
        CHECK(TypeIndex<int> == node->which());
        CHECK(3 == boost::get<int>(*node));
    }
}

TEST_CASE("ConfDefault") {
    Config::Conf c("root");
    c.setDefault(c);

    c.putChild(Config::Conf("child1"));
    c.putChild(Config::Conf("child2"));
    CHECK(2u == c.childCount());

    c.putNode("node1", 123);
    c.putNode("node2", 456);
    CHECK(2u == c.nodeCount());

    c.resetToDefault();
    CHECK("root" == c.name());
    CHECK(0u == c.childCount());
    CHECK(0u == c.nodeCount());
}

TEST_CASE("ConfigManagerTest") {
    const fs::path confPath{Directories::configHome() / "melosic" / "test.conf"};
    Config::Manager confman{confPath};

    REQUIRE((Directories::configHome() / "melosic" / "test.conf") == confPath);
    confman.loadConfig();
    REQUIRE(fs::exists(confPath));

    fs::remove(confPath);
}
