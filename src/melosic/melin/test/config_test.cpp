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

#include <melosic/melin/config.hpp>
#include <melosic/common/directories.hpp>
#include <melosic/common/signal_core.hpp>
using namespace Melosic;

TEST(ConfigTest, ConfVars) {

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
