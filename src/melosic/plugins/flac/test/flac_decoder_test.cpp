#include "../flacdecoder.hpp"
#include <decoder_test.hpp>

MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<FlacDecoder>, "lossless_8_96000_1c.flac");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<FlacDecoder>, "lossless_16_96000_1c.flac");
MELOSIC_INIT_LOSSLESS_DECODER_TEST(std::make_unique<FlacDecoder>, "lossless_24_96000_1c.flac");
