#include "distance_range.h"

#include <algorithm>
#include <limits>

#include "platform.h"
#include "sincos.h"

namespace twim {

void DistanceRange::update(const Vector<int32_t>& region, int32_t angle,
                           const CodecParams& cp) {
  if (region.len == 0) {
    // Exception!
    numLines = -1;
    return;
  }
  const size_t count = region.len;
  const size_t step = region.capacity / 3;
  const int32_t* RESTRICT y = region.data;
  const int32_t* RESTRICT x0 = y + step;
  const int32_t* RESTRICT x1 = x0 + step;

  // Note: kSin is all positive -> d0 <= d1
  int32_t nx = SinCos::kSin[angle];
  int32_t ny = SinCos::kCos[angle];
  int32_t mi = std::numeric_limits<int32_t>::max();
  int32_t ma = std::numeric_limits<int32_t>::min();
  for (size_t i = 0; i < count; i++) {
    int32_t d0 = ny * y[i] + nx * x0[i];
    int32_t d1 = ny * y[i] + nx * (x1[i] - 1);
    mi = std::min(mi, d0);
    ma = std::max(ma, d1);
  }
  min = mi;
  max = ma;

  lineQuant = cp.getLineQuant();
  while (true) {
    numLines = (ma - mi) / lineQuant;
    if (numLines > cp.lineLimit) {
      lineQuant = lineQuant + lineQuant / 16;
    } else {
      break;
    }
  }
}

}  // namespace twim
