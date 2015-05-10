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

#include "catch.hpp"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <jbson/json_reader.hpp>
using namespace jbson;

#define private public

#include <melosic/melin/input.hpp>
#include <melosic/core/track.hpp>
using namespace Melosic;

static boost::exception_ptr to_boost_exception_ptr(std::exception_ptr e) {
    try {
        std::rethrow_exception(e);
    } catch(...) {
        return boost::current_exception();
    }
}

TEST_CASE("Tracks can be constructed with uris & json docs with certain fields") {
    Core::Track track(Input::to_uri("/tmp/some track.flac"));
    CHECK_NOTHROW(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac"
    })"_json_doc});

    CHECK_THROWS(track = Core::Track{R"({
        "type": "track",
        "location": 123
    })"_json_doc});

    CHECK_THROWS(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac",
        "metadata": {}
    })"_json_doc});

    CHECK_THROWS(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac",
        "metadata": [ { "key": "tracknumber", "value": 12 } ]
    })"_json_doc});

    CHECK_NOTHROW(track = Core::Track{R"({
        "type": "track",
        "location": "file:///tmp/some%20track.flac",
        "metadata": [ { "key": "tracknumber", "value": "12" } ]
    })"_json_doc});
}
