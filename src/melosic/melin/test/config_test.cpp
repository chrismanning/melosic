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

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/mpl/index_of.hpp>

#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/common/signal_core.hpp>
using namespace Melosic;

template <typename T>
static constexpr int TypeIndex = boost::mpl::index_of<Config::VarType::types, T>::type::value;

TEST(ConfigTest, ConfVars) {
    Config::VarType var = "The quick brown fox jumped over the lazy dog."s;
    EXPECT_EQ(TypeIndex<std::string>, var.which());
    var = true;
    EXPECT_EQ(TypeIndex<bool>, var.which());
    var = 125U;
    EXPECT_EQ(TypeIndex<uint32_t>, var.which());
    var = -125;
    EXPECT_EQ(TypeIndex<int32_t>, var.which());
    var = 125UL;
    EXPECT_EQ(TypeIndex<uint64_t>, var.which());
    var = -125L;
    EXPECT_EQ(TypeIndex<int64_t>, var.which());
    var = 6573.738504;
    EXPECT_EQ(TypeIndex<double>, var.which());
}

TEST(ConfigTest, ConfNodes) {
    const std::string name("Test Conf Tree");
    Config::Conf c(name);
    EXPECT_EQ(name, c.getName());
    EXPECT_EQ(0u, c.nodeCount());

    std::string str_node("adf");
    c.putNode("str_key", str_node);
    EXPECT_EQ(1u, c.nodeCount());
    auto node = c.getNode("str_key")->second;
    EXPECT_EQ(TypeIndex<std::string>, node.which());
    EXPECT_EQ(str_node, boost::get<std::string>(node));

    c.putNode("str_key", 24564235465UL);
    node = c.getNode("str_key")->second;
    EXPECT_EQ(TypeIndex<uint64_t>, node.which());
    EXPECT_EQ(24564235465UL, boost::get<uint64_t>(node));

    c.removeNode("str_key");
    EXPECT_EQ(0u, c.nodeCount());
}

TEST(ConfigTest, ConfChild) {
    Config::Conf base("root");
    const std::string name("Test Conf Tree");
    Config::Conf tmp(name);
    base.putChild(std::move(tmp));
    EXPECT_EQ(0u, base.nodeCount());
    EXPECT_EQ(1u, base.childCount());

    auto child = base.getChild(name);
    ASSERT_NE(nullptr, child);
    EXPECT_EQ(name, child->getName());

    base.removeChild(name);
    EXPECT_EQ(0u, base.childCount());
}

TEST(ConfigTest, ConfCopy) {
    Config::Conf c1("root sdf asdr");
    c1.putNode("test", 1066);
    EXPECT_EQ(1u, c1.nodeCount());
    Config::Conf c2(c1);
    EXPECT_EQ(1u, c2.nodeCount());

    auto node = c1.getNode("test")->second;
    EXPECT_EQ(TypeIndex<int32_t>, node.which());
    EXPECT_EQ(1066, boost::get<int32_t>(node));

    c1.removeNode("test");
    EXPECT_EQ(0u, c1.nodeCount());
    EXPECT_EQ(1u, c2.nodeCount());
}

TEST(ConfigTest, ConfMerge) {
    Config::Conf c1("root");
    c1.putNode("PI", 3.1415926);
    c1.putNode("PI_Str", "3.1415926"s);
    EXPECT_EQ(2u, c1.nodeCount());

    Config::Conf c2("root2");
    c2.putNode("PI_Int", 3);

    c1.merge(c2);
    EXPECT_EQ(1u, c2.nodeCount());
    EXPECT_EQ(3u, c1.nodeCount());

    auto node = c1.getNode("PI");
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(TypeIndex<double>, node->second.which());
    EXPECT_EQ(3.1415926, boost::get<double>(node->second));

    node = c1.getNode("PI_Str");
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(TypeIndex<std::string>, node->second.which());
    EXPECT_EQ("3.1415926", boost::get<std::string>(node->second));

    node = c1.getNode("PI_Int");
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(TypeIndex<int>, node->second.which());
    EXPECT_EQ(3, boost::get<int>(node->second));
}

struct ConfigManagerTest : ::testing::Test {
    ~ConfigManagerTest() {
        fs::remove(confPath);
    }

    fs::path confPath{Directories::configHome() / "melosic" / "test.conf"};
    Config::Manager confman{confPath};
};

TEST_F(ConfigManagerTest, ConfigManagerTest1) {
    ASSERT_EQ(Directories::configHome() / "melosic" / "test.conf", confPath);
    confman.loadConfig();
    ASSERT_TRUE(fs::exists(confPath)) << "config file not found:" << confPath;
}
