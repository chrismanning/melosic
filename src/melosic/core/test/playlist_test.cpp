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
#include <boost/utility/string_ref.hpp>
using namespace boost::literals;

#define private public

#include <melosic/core/playlist.hpp>
#include <melosic/melin/input.hpp>
#include <melosic/core/track.hpp>
using namespace Melosic;

TEST_CASE("Playlist Test 1") {
    Core::Playlist playlist{"Playlist 1"};
    auto track = Core::Track{Input::to_uri("/tmp/some track.flac")};
    CHECK("file:///tmp/some%20track.flac" == track.uri().string());

    auto end = playlist.end();
    REQUIRE(end == playlist.begin());

    auto i = playlist.insert(0, track);
    REQUIRE(playlist.end() == end);
    CHECK(playlist.begin() != end);
    CHECK(track == *playlist.begin());
    CHECK(track == playlist.getTrack(i));
    REQUIRE(playlist.end() == end);

    auto track2 = Core::Track{Input::to_uri("/tmp/some other track.flac")};
    CHECK("file:///tmp/some%20other%20track.flac" == track2.uri().string());

    i = playlist.insert(0, track2);
    REQUIRE(playlist.end() == end);
    CHECK(playlist.begin() != end);
    CHECK(track == *playlist.begin());
    CHECK(track2 == playlist.getTrack(i));
    REQUIRE(playlist.end() == end);
}
