#ifndef TWIM_REGION
#define TWIM_REGION

#include "platform.h"

namespace twim {

class Region {
 public:
  static void splitLine(const Vector<int32_t>& region, int32_t angle, int32_t d,
                        Vector<int32_t>* left, Vector<int32_t>* right);
};

}  // namespace twim

#endif  // TWIM_REGION