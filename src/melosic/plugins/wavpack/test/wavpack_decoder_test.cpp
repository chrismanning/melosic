#include "../wavpackdecoder.hpp"
#include "../wavpack_provider.hpp"
#include <decoder_test.hpp>

using namespace wavpack;

MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_8_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_16_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_24_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_32_96000_1c.wv");

TEST_CASE("verify by checksums") {
    const boost::filesystem::path test_data_dir{MELOSIC_TEST_DATA_DIR "/wavpack_test_suite"};
    provider provider;
    boost::filesystem::path path;

    SECTION("8bit") {
        path = test_data_dir / "bit_depths/8bit.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }

    SECTION("12bit") {
        path = test_data_dir / "bit_depths/12bit.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }

    SECTION("16bit") {
        path = test_data_dir / "bit_depths/16bit.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }

    SECTION("20bit") {
        path = test_data_dir / "bit_depths/20bit.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }

    SECTION("24bit") {
        path = test_data_dir / "bit_depths/24bit.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }

    SECTION("32bit") {
        path = test_data_dir / "bit_depths/32bit_int.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }

    SECTION("mono") {
        path = test_data_dir / "num_channels/mono-1.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }

    SECTION("stereo") {
        path = test_data_dir / "num_channels/stereo-2.wv";
        CHECK(provider.verify(std::make_unique<boost::filesystem::ifstream>(path)));
    }
}
