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

#include <chrono>
using namespace std::literals;

#include <catch.hpp>

#include <melosic/common/audiospecs.hpp>
using AS = Melosic::AudioSpecs;

TEST_CASE("SamplesToBytesTest") {
    AS as{1, 8, 44100};
    CHECK(44100u == as.samples_to_bytes(44100));
    as = {2, 8, 44100};
    CHECK(88200u == as.samples_to_bytes(44100));
    as = {2, 16, 44100};
    CHECK(176400u == as.samples_to_bytes(44100));

    CHECK(40u == as.samples_to_bytes(10));
}

TEST_CASE("BytesToSamplesTest") {
    AS as{1, 8, 44100};
    CHECK(44100u == as.bytes_to_samples(44100));
    as = {2, 8, 44100};
    CHECK(44100u == as.bytes_to_samples(88200));
    as = {2, 16, 44100};
    CHECK(44100u == as.bytes_to_samples(176400u));
    CHECK(10u == as.bytes_to_samples(40));
}

constexpr AS as{2, 16, 44100};
static_assert(22050u == as.time_to_samples(500ms), "");

TEST_CASE("TimeToSamplesTest") {
    constexpr AS as{2, 16, 44100};
    static_assert(22050u == as.time_to_samples(500ms), "");

    CHECK(44100u == as.time_to_samples(1s));
    CHECK(44100u == as.time_to_samples(1000ms));
    CHECK(22050u == as.time_to_samples(500ms));
}

TEST_CASE("SamplesToTimeTest") {
    constexpr AS as{2, 16, 44100};
    static_assert(500ms == as.samples_to_time<chrono::milliseconds>(22050u), "");

    CHECK(1s == as.samples_to_time<chrono::seconds>(44100u));
    CHECK(500ms == as.samples_to_time<chrono::milliseconds>(22050u));
}
