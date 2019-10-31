#include "image.h"

namespace twim {

Image Image::fromRgba(const uint8_t* src, uint32_t width, uint32_t height) {
  Image result;
  result.width = width;
  result.height = height;

  result.r.resize(width * height);
  result.g.resize(width * height);
  result.b.resize(width * height);

  for (size_t y = 0; y < height; ++y) {
    const uint8_t* from = src + y * 4 * width;
    uint8_t* to_r = result.r.data() + y * width;
    uint8_t* to_g = result.g.data() + y * width;
    uint8_t* to_b = result.b.data() + y * width;
    for (size_t x = 0; x < width; ++x) {
      to_r[x] = from[4 * x];
      to_g[x] = from[4 * x + 1];
      to_b[x] = from[4 * x + 2];
    }
  }

  return result;
}

}  // namespace twim
