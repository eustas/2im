#include "image.h"

namespace twim {

void Image::init(uint32_t width, uint32_t height) {
  this->r_.resize(width * height);
  this->g_.resize(width * height);
  this->b_.resize(width * height);

  this->width = width;
  this->height = height;

  this->r = this->r_.data();
  this->g = this->g_.data();
  this->b = this->b_.data();

  this->ok = true;
}

Image Image::fromRgba(const uint8_t* src, uint32_t width, uint32_t height) {
  Image result;
  result.init(width, height);

  for (size_t y = 0; y < height; ++y) {
    const uint8_t* from = src + y * 4 * width;
    uint8_t* to_r = result.r + y * width;
    uint8_t* to_g = result.g + y * width;
    uint8_t* to_b = result.b + y * width;
    for (size_t x = 0; x < width; ++x) {
      to_r[x] = from[4 * x];
      to_g[x] = from[4 * x + 1];
      to_b[x] = from[4 * x + 2];
    }
  }

  return result;
}

}  // namespace twim
