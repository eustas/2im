#include "distance_range.h"

#include <algorithm>
#include <limits>

#include "platform.h"
#include "sin_cos.h"

namespace twim {

DistanceRange::DistanceRange(const Vector<int32_t>& region, int32_t angle,
                             const CodecParams& cp) {
  if (region.len == 0) {
    // Exception!
    num_lines = kInvalid;
    return;
  }
  const size_t count = region.len;
  const size_t step = region.capacity / 3;
  const int32_t* RESTRICT y = region.data();
  const int32_t* RESTRICT x0 = y + step;
  const int32_t* RESTRICT x1 = x0 + step;

  // Note: kSin is all positive -> d0 <= d1
  int32_t nx = SinCos.kSin[angle];
  int32_t ny = SinCos.kCos[angle];
  int32_t mi = std::numeric_limits<int32_t>::max();
  int32_t ma = std::numeric_limits<int32_t>::min();
  for (size_t i = 0; i < count; i++) {
    int32_t d0 = ny * y[i] + nx * x0[i];
    int32_t d1 = ny * y[i] + nx * (x1[i] - 1);
    mi = std::min(mi, d0);
    ma = std::max(ma, d1);
  }
  // TODO(eustas): Check >= 0
  min = static_cast<uint32_t>(mi);
  // TODO(eustas): Check >= 0
  max = static_cast<uint32_t>(ma);

  line_quant = cp.getLineQuant();
  uint32_t delta = ma - mi;
  if (delta > line_quant * cp.line_limit) line_quant = (2 * delta + cp.line_limit) / (2 * cp.line_limit);
  num_lines = std::max<uint32_t>(delta / line_quant, 1);
}

}  // namespace twim
