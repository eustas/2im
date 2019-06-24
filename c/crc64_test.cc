#include "crc64.h"
#include "gtest/gtest.h"

namespace twim {

TEST(Crc64Test, Abc) {
  uint64_t crc = initCrc64();
  for (size_t i = 0; i < 10; ++i) {
    crc = updateCrc64(crc, 97 + i);
  }
  std::string result = finishCrc64(crc);
  EXPECT_EQ("32093A2ECD5773F4", result);
}

}  // namespace twim
