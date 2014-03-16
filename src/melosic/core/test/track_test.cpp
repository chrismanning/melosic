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

#include <jbson/json_reader.hpp>
using namespace jbson;

#define private public

#include <melosic/melin/input.hpp>
#include <melosic/core/track.hpp>
using namespace Melosic;

TEST(TrackTest, TestTrack1) {
    auto track = Core::Track{Input::to_uri("/tmp/some track.flac")};
    EXPECT_EQ("file:///tmp/some%20track.flac", track.uri().string());
}

static boost::exception_ptr to_boost_exception_ptr(std::exception_ptr e) {
    try {
        std::rethrow_exception(e);
    }
    catch(...) {
        return boost::current_exception();
    }
}

TEST(TrackTest, TestTrack2) {
    Core::Track track(Input::to_uri("/tmp/some track.flac"));
    EXPECT_NO_THROW(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac"
    })"_json_doc});

    EXPECT_ANY_THROW(track = Core::Track{R"({
        "type": "track",
        "location": 123
    })"_json_doc});

    EXPECT_ANY_THROW(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac",
        "metadata": {}
    })"_json_doc});

    EXPECT_ANY_THROW(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac",
        "metadata": [ { "key": "tracknumber", "value": 12 } ]
    })"_json_doc});

    EXPECT_NO_THROW(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac",
        "metadata": [ { "key": "tracknumber", "value": "12" } ]
    })"_json_doc});
}
