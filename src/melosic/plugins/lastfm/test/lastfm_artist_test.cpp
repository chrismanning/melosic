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

#include <lastfm/artist.hpp>
#include <lastfm/tag.hpp>

TEST_CASE("artist_deserialise") {
    boost::filesystem::path test_dir{MELOSIC_TEST_DATA_DIR};
    SECTION("get_info") {
        boost::filesystem::ifstream is{test_dir / "artist_getinfo.json"};
        std::string artist_json;
        REQUIRE(std::getline(is, artist_json, static_cast<char>(EOF)));

        auto doc = jbson::read_json(artist_json);
        REQUIRE(boost::size(doc) == 1);

        auto artist_elem = *doc.begin();

        lastfm::artist artist;
        REQUIRE_NOTHROW(artist = jbson::get<lastfm::artist>(artist_elem));

        CHECK(artist.name() == "Metallica");
        CHECK(artist.top_tags().front().name() == "thrash metal");
        CHECK_FALSE(artist.streamable());
        CHECK(artist.wiki().summary().size() > 0);
        REQUIRE(artist.images().size() > 0);
        CHECK(artist.images().back().size() != lastfm::image_size::small);
    }
}
