#include "sin_cos.h"

#include "crc64.h"
#include "gtest/gtest.h"

namespace twim {

TEST(SinCosTest, Sin) {
  uint64_t crc = Crc64::init();
  for (size_t i = 0; i < SinCos::kMaxAngle; ++i) {
    crc = Crc64::update(crc, SinCos::kSin[i] & 0xFF);
  }
  EXPECT_EQ("9486473C3841E28F", Crc64::finish(crc));
}

TEST(SinCosTest, Cos) {
  uint64_t crc = Crc64::init();
  for (size_t i = 0; i < SinCos::kMaxAngle; ++i) {
    crc = Crc64::update(crc, SinCos::kCos[i] & 0xFF);
  }
  EXPECT_EQ("A32700985A177AE9", Crc64::finish(crc));
}

}  // namespace twim
