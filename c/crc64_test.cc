#include "crc64.h"

#include "gtest/gtest.h"

namespace twim {

TEST(Crc64Test, Abc) {
  uint64_t crc = Crc64::init();
  for (size_t i = 0; i < 10; ++i) {
    crc = Crc64::update(crc, 97 + i);
  }
  EXPECT_EQ("32093A2ECD5773F4", Crc64::finish(crc));
}

}  // namespace twim
