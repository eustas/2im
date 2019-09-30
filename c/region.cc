#include "region.h"

#include "platform.h"
#include "sin_cos.h"

namespace twim {

void Region::splitLine(const Vector<int32_t>& region, int32_t angle, int32_t d,
                       Vector<int32_t>* left, Vector<int32_t>* right) {
  const size_t region_step = region.capacity / 3;
  const size_t region_count = region.len;
  const int32_t* RESTRICT region_y = region.data();
  const int32_t* RESTRICT region_x0 = region_y + region_step;
  const int32_t* RESTRICT region_x1 = region_x0 + region_step;

  const size_t left_step = left->capacity / 3;
  int32_t* RESTRICT left_y = left->data();
  int32_t* RESTRICT left_x0 = left_y + left_step;
  int32_t* RESTRICT left_x1 = left_x0 + left_step;

  const size_t right_step = right->capacity / 3;
  int32_t* RESTRICT right_y = right->data();
  int32_t* RESTRICT right_x0 = right_y + right_step;
  int32_t* RESTRICT right_x1 = right_x0 + right_step;

  int32_t nx = SinCos::kSin[angle];
  int32_t ny = SinCos::kCos[angle];
  uint32_t l_count = 0;
  uint32_t r_count = 0;

  if (nx == 0) {
    // nx = 0 -> ny = SinCos.SCALE -> y * ny ?? d
    for (size_t i = 0; i < region_count; i++) {
      int32_t y = region_y[i];
      int32_t x0 = region_x0[i];
      int32_t x1 = region_x1[i];
      if (y * ny >= d) {
        left_y[l_count] = y;
        left_x0[l_count] = x0;
        left_x1[l_count] = x1;
        l_count++;
      } else {
        right_y[r_count] = y;
        right_x0[r_count] = x0;
        right_x1[r_count] = x1;
        r_count++;
      }
    }
  } else {
    // nx > 0 -> x ?? (d - y * ny) / nx
    d = 2 * d + nx;
    ny = 2 * ny;
    nx = 2 * nx;
    for (size_t i = 0; i < region_count; i++) {
      int32_t y = region_y[i];
      int32_t x = (d - y * ny) / nx;
      int32_t x0 = region_x0[i];
      int32_t x1 = region_x1[i];
      if (x < x1) {
        left_y[l_count] = y;
        left_x0[l_count] = std::max(x, x0);
        left_x1[l_count] = x1;
        l_count++;
      }
      if (x > x0) {
        right_y[r_count] = y;
        right_x0[r_count] = x0;
        right_x1[r_count] = std::min(x, x1);
        r_count++;
      }
    }
  }
  left->len = l_count;
  right->len = r_count;
}

}  // namespace twim
