#ifndef TWIM_DISTANCE_RANGE
#define TWIM_DISTANCE_RANGE

#include "codec_params.h"
#include "platform.h"

namespace twim {

class DistanceRange {
 public:
  void update(const Vector<int32_t>& region, int32_t angle,
              const CodecParams& cp);

  uint32_t INLINE distance(uint32_t line) {
    if (num_lines > 1) {
      return min + ((max - min) - (num_lines - 1) * line_quant) / 2 +
             line_quant * line;
    } else {
      return (max + min) / 2;
    }
  }

  static const uint32_t kInvalid = static_cast<uint32_t>(-1);
  uint32_t num_lines;

 private:
  uint32_t min;
  uint32_t max;
  uint32_t line_quant;
};

}  // namespace twim

#endif  // TWIM_DISTANCE_RANGE
