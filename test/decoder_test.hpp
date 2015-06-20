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

#include <catch.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>
#include <asio/error.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <melosic/common/pcmbuffer.hpp>

inline static Melosic::AudioSpecs get_specs_from_name(const boost::filesystem::path& filename) {
    static const boost::regex e("_([0-9]+)_([0-9]+)_([0-9])c", boost::regex::extended);
    boost::smatch matches;
    REQUIRE(boost::regex_search(filename.string(), matches, e));
    REQUIRE(matches.size() == 4);
    Melosic::AudioSpecs as;
    as.bps = std::stoi(matches[1]);
    as.sample_rate = std::stoi(matches[2]);
    as.channels = std::stoi(matches[3]);
    return as;
}

#define MELOSIC_INIT_LOSSLESS_DECODER_TEST(decoder_factory, test_filename)                                             \
    TEST_CASE("decode test: " test_filename) {                                                                         \
        const boost::filesystem::path test_data_dir{MELOSIC_TEST_DATA_DIR};                                            \
        REQUIRE(boost::filesystem::exists(test_data_dir));                                                             \
                                                                                                                       \
        const auto path = test_data_dir / test_filename;                                                               \
        REQUIRE(boost::filesystem::exists(path));                                                                      \
                                                                                                                       \
        auto reference_path = path;                                                                                    \
        reference_path.replace_extension(".pcm");                                                                      \
        REQUIRE(boost::filesystem::exists(reference_path));                                                            \
                                                                                                                       \
        boost::filesystem::ifstream reference_file{reference_path};                                                    \
        std::vector<char> reference_pcm(boost::filesystem::file_size(reference_path), 0);                              \
        REQUIRE_NOTHROW(reference_file.read(reference_pcm.data(), reference_pcm.size()));                              \
                                                                                                                       \
        auto decoder = decoder_factory(std::make_unique<boost::filesystem::ifstream>(path));                           \
        const auto as = decoder->getAudioSpecs();                                                                      \
        REQUIRE(get_specs_from_name(path.filename()) == as);                                                           \
        CHECK(decoder->duration().count() == 1000);                                                                    \
        std::error_code ec;                                                                                            \
                                                                                                                       \
        SECTION("full decode") {                                                                                       \
            std::vector<char> decoded_pcm(reference_pcm.size(), 0);                                                    \
            Melosic::PCMBuffer buf{decoded_pcm.data(), decoded_pcm.size()};                                            \
            REQUIRE_NOTHROW(CHECK(as.time_to_bytes(decoder->duration()) == decoder->decode(buf, ec)));                 \
            REQUIRE(!ec);                                                                                              \
                                                                                                                       \
            CHECK(decoded_pcm == reference_pcm);                                                                       \
                                                                                                                       \
            REQUIRE_NOTHROW(CHECK(0 == decoder->decode(buf, ec)));                                                     \
            CHECK(ec.value() == asio::error::eof);                                                                     \
        }                                                                                                              \
        SECTION("half decode") {                                                                                       \
            std::vector<char> decoded_pcm(reference_pcm.size() / 2, 0);                                                \
            Melosic::PCMBuffer buf{decoded_pcm.data(), decoded_pcm.size()};                                            \
            REQUIRE_NOTHROW(CHECK(as.time_to_bytes(decoder->duration() / 2) == decoder->decode(buf, ec)));             \
            REQUIRE(!ec);                                                                                              \
                                                                                                                       \
            CHECK(boost::equal(decoded_pcm, reference_pcm | boost::adaptors::sliced(0, decoded_pcm.size())));          \
                                                                                                                       \
            REQUIRE_NOTHROW(CHECK(as.time_to_bytes(decoder->duration() / 2) == decoder->decode(buf, ec)));             \
            REQUIRE(!ec);                                                                                              \
                                                                                                                       \
            CHECK(boost::equal(decoded_pcm,                                                                            \
                               reference_pcm | boost::adaptors::sliced(decoded_pcm.size(), reference_pcm.size())));    \
                                                                                                                       \
            REQUIRE_NOTHROW(CHECK(0 == decoder->decode(buf, ec)));                                                     \
            CHECK(ec.value() == asio::error::eof);                                                                     \
        }                                                                                                              \
        SECTION("seek") {                                                                                              \
            std::vector<char> decoded_pcm(reference_pcm.size() / 2, 0);                                                \
            Melosic::PCMBuffer buf{decoded_pcm.data(), decoded_pcm.size()};                                            \
                                                                                                                       \
            REQUIRE_NOTHROW(decoder->seek(500ms));                                                                     \
                                                                                                                       \
            REQUIRE_NOTHROW(CHECK(as.time_to_bytes(decoder->duration() / 2) == decoder->decode(buf, ec)));             \
            REQUIRE(!ec);                                                                                              \
                                                                                                                       \
            CHECK(boost::equal(decoded_pcm,                                                                            \
                               reference_pcm | boost::adaptors::sliced(decoded_pcm.size(), reference_pcm.size())));    \
                                                                                                                       \
            REQUIRE_NOTHROW(decoder->seek(0ms));                                                                       \
                                                                                                                       \
            REQUIRE_NOTHROW(CHECK(as.time_to_bytes(decoder->duration() / 2) == decoder->decode(buf, ec)));             \
            REQUIRE(!ec);                                                                                              \
                                                                                                                       \
            CHECK(boost::equal(decoded_pcm, reference_pcm | boost::adaptors::sliced(0, decoded_pcm.size())));          \
        }                                                                                                              \
    }
