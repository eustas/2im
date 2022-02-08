#include "sin_cos.h"

#include <cmath>

#include "crc64.h"
#include "gtest/gtest.h"

namespace twim {

TEST(SinCosTest, Sin) {
  uint64_t crc = Crc64::init();
  for (size_t i = 0; i < SinCos.kMaxAngle; ++i) {
    crc = Crc64::update(crc, SinCos.kSin[i] & 0xFF);
  }
  EXPECT_EQ("9486473C3841E28F", Crc64::finish(crc));
}

TEST(SinCosTest, Cos) {
  uint64_t crc = Crc64::init();
  for (size_t i = 0; i < SinCos.kMaxAngle; ++i) {
    crc = Crc64::update(crc, SinCos.kCos[i] & 0xFF);
  }
  EXPECT_EQ("A32700985A177AE9", Crc64::finish(crc));
}

TEST(SinCosTest, Log2) {
  for (size_t i = 1; i < 64; ++i) {
    float diff = (std::log(i) / std::log(2)) - SinCos.kLog2[i];
    EXPECT_LE(std::abs(diff), 0.00000022f) << i;
  }
}

// TODO(eustas): add Pow2 test

}  // namespace twim
