/**************************************************************************
**  Copyright (C) 2015 Christian Manning
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

#ifndef MELOSIC_TEST_DATA_DIR
#error "MELOSIC_TEST_DATA_DIR define needed"
#endif

#include "catch.hpp"

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/range/size.hpp>

#include <jbson/json_reader.hpp>

#include <lastfm/track.hpp>

TEST_CASE("deserialise") {
    boost::filesystem::path test_dir{MELOSIC_TEST_DATA_DIR};
    SECTION("get_info") {
        boost::filesystem::ifstream is{test_dir/"track_getinfo.json"};
        std::string track_json;
        REQUIRE(std::getline(is, track_json, static_cast<char>(EOF)));

        auto doc = jbson::read_json(track_json);
        REQUIRE(boost::size(doc) == 1);

        auto track_elem = *doc.begin();

        lastfm::track track;
        REQUIRE_NOTHROW(track = jbson::get<lastfm::track>(track_elem));

        CHECK(track.name() == "Master of Puppets");
        CHECK(track.artist().name() == "Metallica");
        CHECK(track.album().name() == "Master of Puppets");
        CHECK(track.tracknumber() == 2);
        REQUIRE(track.tags().size() > 0);
        CHECK(track.tags().front().name() == "thrash metal");
        CHECK_FALSE(track.streamable());
        CHECK(track.wiki().summary().size() > 0);
    }
}
