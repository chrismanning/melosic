#include "../wavpackdecoder.hpp"
#include <decoder_test.hpp>

MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<WavpackDecoder>, "lossless_8_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<WavpackDecoder>, "lossless_16_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<WavpackDecoder>, "lossless_24_96000_1c.wv");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<WavpackDecoder>, "lossless_32_96000_1c.wv");
