#include "gtest/gtest.h"
#include "xrange_decoder.h"
#include "xrange_encoder.h"

#include <cstring>  /* memcmp */

namespace twim {

static uint32_t Rng(uint32_t* state) {
  uint64_t next = *state;
  next = (next * 16807u) % 0x7FFFFFFFu;
  *state = static_cast<uint32_t>(next);
  return *state;
}

void TestRandom(size_t num_rounds, size_t num_items, size_t seed) {
  uint32_t rng = seed;
  std::vector<uint32_t> items(num_items * 2);
  for (size_t r = 0; r < num_rounds; ++r) {
    XRangeEncoder encoder;
    for (size_t i = 0; i < num_items * 2; i += 2) {
      uint32_t total = 1 + Rng(&rng) % 42;
      uint32_t val = Rng(&rng) % total;
      XRangeEncoder::writeNumber(&encoder, total, val);
      items[i] = val;
      items[i + 1] = total;
    }
    Array<uint8_t> data(1024);
    encoder.finish(&data);

    XRangeDecoder decoder(std::vector<uint8_t>(data.data, data.data + data.size));
    for (size_t i = 0; i < num_items * 2; i += 2) {
      uint32_t val = XRangeDecoder::readNumber(&decoder, items[i + 1]);
      ASSERT_EQ(items[i], val);
    }
  }
}

TEST(XRangeTest, Random10) { TestRandom(1000, 10, 42); }
TEST(XRangeTest, Random30) { TestRandom(1000, 30, 44); }
TEST(XRangeTest, Random50) { TestRandom(1000, 50, 46); }
TEST(XRangeTest, Random70) { TestRandom(1000, 70, 48); }
TEST(XRangeTest, Random90) { TestRandom(1000, 90, 50); }
TEST(XRangeTest, Random10000000) { TestRandom(1, 10000000, 51); }

TEST(XRangeTest, Optimizer) {
  XRangeEncoder encoder;
  const size_t kLength = 12;
  // 2 bits of overhead is bearable; amount depends on luck.
  XRangeEncoder::writeNumber(&encoder, 63, 13);
  for (uint32_t i = 0; i < kLength - 1; ++i) {
    XRangeEncoder::writeNumber(&encoder, 256, i + 42);
  }
  Array<uint8_t> data(1024);
  encoder.finish(&data);

  int8_t expected[] = {-78, -15, 81, -45, -48, -46, -47, 51, 48, 50, 49, -77};
  const size_t expected_size = sizeof(expected) / sizeof(expected[0]);
  ASSERT_EQ(kLength, expected_size);
  EXPECT_EQ(expected_size, data.size);
  EXPECT_EQ(0, std::memcmp(expected, data.data, sizeof(expected)));

  XRangeDecoder decoder(std::vector<uint8_t>(data.data, data.data + data.size));
  EXPECT_EQ(13u, XRangeDecoder::readNumber(&decoder, 63));
  for (size_t i = 0; i < kLength - 1; ++i) {
    EXPECT_EQ(i + 42, XRangeDecoder::readNumber(&decoder, 256));
  }
}

}  // namespace twim
