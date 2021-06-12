#include "codec_params.h"

#include "gtest/gtest.h"
#include "xrange_encoder.h"

namespace twim {

// Actually methods reside in encoder, but...
void writeSize(XRangeEncoder* dst, uint32_t value);
uint32_t simulateWriteSize(uint32_t value);
float calculateImageTax(uint32_t width, uint32_t height);

class XRangeEncoderFriend {
 public:
  static uint32_t countBits(XRangeEncoder* src) {
    uint32_t count = 0;
    for (size_t i = 0; i < src->entries.size; ++i) {
      uint32_t max = src->entries.data[i].max;
      if ((max & (max - 1)) != 0) return -1;
      while (max > 1) {
        count++;
        max >>= 1;
      }
    }
    return count;
  }
  static float estimateEntropy(XRangeEncoder* src) {
    float v = 1.0f;
    for (size_t i = 0; i < src->entries.size; ++i) {
      v *= src->entries.data[i].max;
    }
    return log2(v);
  }
};

TEST(CodecParamsTest, WriteSize) {
  for (size_t i = 8; i <= 2048; ++i) {
    XRangeEncoder dst;
    writeSize(&dst, i);
    EXPECT_EQ(XRangeEncoderFriend::countBits(&dst), simulateWriteSize(i)) << i;
  }
}

TEST(CodecParamsTest, ImageTax) {
  XRangeEncoder dst;
  CodecParams cp(8, 8);
  cp.write(&dst);
  EXPECT_EQ(XRangeEncoderFriend::estimateEntropy(&dst),
            calculateImageTax(8, 8));
}
}  // namespace twim
