#include "../wavpackdecoder.hpp"
#include <decoder_test.hpp>

using namespace wavpack;

MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_8_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_16_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_24_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<wavpack_decoder>, "lossless_32_96000_1c.wv");
