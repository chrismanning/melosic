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

#include <lastfm/album.hpp>
#include <lastfm/track.hpp>
#include <lastfm/tag.hpp>

TEST_CASE("album_deserialise") {
    boost::filesystem::path test_dir{MELOSIC_TEST_DATA_DIR};
    SECTION("get_info") {
        boost::filesystem::ifstream is{test_dir / "album_getinfo.json"};
        std::string album_json;
        REQUIRE(std::getline(is, album_json, static_cast<char>(EOF)));

        auto doc = jbson::read_json(album_json);
        REQUIRE(boost::size(doc) == 1);

        auto album_elem = *doc.begin();

        lastfm::album album;
        REQUIRE_NOTHROW(album = jbson::get<lastfm::album>(album_elem));

        CHECK(album.name() == "Master of Puppets");
        CHECK(album.artist().name() == "Metallica");
        REQUIRE(album.top_tags().size() > 0);
        CHECK(album.top_tags().front().name() == "thrash metal");
        CHECK_FALSE(album.streamable());
        CHECK(album.tracks().size() > 0);
        REQUIRE(album.images().size() > 0);
        CHECK(album.images().back().size() != lastfm::image_size::small);
    }
}
