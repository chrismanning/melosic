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

#include <lastfm/event.hpp>
#include <lastfm/artist.hpp>

TEST_CASE("album_deserialise") {
    boost::filesystem::path test_dir{MELOSIC_TEST_DATA_DIR};
    SECTION("get_info") {
        boost::filesystem::ifstream is{test_dir/"event_getinfo.json"};
        std::string event_json;
        REQUIRE(std::getline(is, event_json, static_cast<char>(EOF)));

        auto doc = jbson::read_json(event_json);
        REQUIRE(boost::size(doc) == 1);

        auto event_elem = *doc.begin();

        lastfm::event event;
        try {
        /*REQUIRE_NOTHROW*/(event = jbson::get<lastfm::event>(event_elem));
        }
        catch(...) {
            std::clog << boost::current_exception_diagnostic_information();
            CHECK(false);
        }

        CHECK(event.name() == "Philip Glass");
        REQUIRE(event.artists().size() > 0);
        CHECK(event.artists().front().name() == "Philip Glass");
        CHECK(event.images().size() > 0);
        CHECK(event.venue().name() == "Barbican Centre");
        CHECK(event.venue().images().size() > 0);
    }
}
