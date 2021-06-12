#include "encoder.h"

#include "gtest/gtest.h"

namespace twim {

namespace {
uint32_t getCrossPixel(size_t x, size_t y) {
  return (0x23880 >> ((x >> 2) + 5 * (y >> 2)) & 1) == 1 ? 0xFFFFFFFF : 0xFF0000FF;
}
Image makeCross() {
  std::vector<uint32_t> tmp(20 * 20);
  for (size_t y = 0; y < 20; ++y) {
    for (size_t x = 0; x < 20; ++x) {
      tmp[y * 20 + x] = getCrossPixel(x, y);
    }
  }
  return Image::fromRgba(reinterpret_cast<uint8_t*>(tmp.data()), 20, 20);
}
}  // namespace

TEST(EncoderTest, EncodeCross) {
  Encoder::Params params;
  params.targetSize = 24;
  Encoder::Variant variant;
  variant.partitionCode = 0xD7;
  variant.lineLimit = 6;
  variant.colorOptions = 1 << 18;
  params.variants = &variant;
  params.numVariants = 1;
  auto result = Encoder::encode(makeCross(), params);
  ASSERT_LE(result.mse, 1e-3f);
  ASSERT_LE(result.data.size, 23u);
  /*fprintf(stderr, "mse: %f, size: %zu\n", result.mse, result.data.size);
  for (size_t i = 0; i < result.data.size; ++i) {
    fprintf(stderr, "%d, ", static_cast<int8_t>(result.data.data[i]));
  }
  fprintf(stderr, "\n");
  ASSERT_TRUE(false);*/
}

}  // namespace twim
