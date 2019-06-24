#ifndef TWIM_CRC64
#define TWIM_CRC64

#include <string>

#include "platform.h"

namespace {

constexpr const uint64_t kCrc64Poly = UINT64_C(0xC96C5795D7870F42);

}  // namespace

namespace twim {

/**
* Roll CRC64 calculation.
*
* <p> {@code CRC64(data) = finish(update((... update(init(), firstByte), ...), lastByte));}
* <p> This simple and reliable checksum is chosen to make is easy to calculate the same value
* across the variety of languages (C++, Java, Go, ...).
*/
INLINE uint64_t updateCrc64(uint64_t crc, uint8_t next) {
  uint64_t c = (crc ^ next) & 0xFF;
  for (size_t i = 0; i < 8; ++i) {
    bool b = ((c & 1) == 1);
    uint64_t d = c >> 1;
    c = b ? (kCrc64Poly ^ d) : d;
  }
  return c ^ (crc >> 8);
}

INLINE uint64_t initCrc64() {
  return (uint64_t)(-1);
}

INLINE std::string finishCrc64(uint64_t crc) {
  crc = ~crc;
  std::string result(16, '0');
  static char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  for (size_t i = 0; i < 16; ++i) {
    result[15 - i] = hex[crc & 0xF];
    crc >>= 4;
  }
  return result;
}

}  // namespace twim

#endif  // TWIM_CRC64
