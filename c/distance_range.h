#ifndef TWIM_DISTANCE_RANGE
#define TWIM_DISTANCE_RANGE

#include "codec_params.h"
#include "platform.h"

namespace twim {

class DistanceRange {
 public:
  void update(const Vector<int32_t>& region, int32_t angle,
              const CodecParams& cp);

  int32_t INLINE distance(int32_t line) {
    if (numLines > 1) {
      return min + ((max - min) - (numLines - 1) * lineQuant) / 2 +
             lineQuant * line;
    } else {
      return (max + min) / 2;
    }
  }
  int32_t numLines;

 private:
  int32_t min;
  int32_t max;
  int32_t lineQuant;
};

}  // namespace twim

#endif  // TWIM_DISTANCE_RANGE
