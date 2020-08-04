#include "sin_cos.h"

#include <cmath>

namespace twim {

// sin / cos functions are WASM fatters; replace with polynomial approximation
// of delta plus correction.
static const uint8_t kSinCorrection[] = {
    14, 16, 16, 18, 19, 20, 20, 23, 23, 25, 25, 27, 27, 28, 29, 32, 31, 32, 33,
    35, 35, 36, 36, 38, 38, 39, 39, 41, 41, 42, 42, 44, 43, 45, 45, 45, 46, 46,
    47, 49, 48, 49, 48, 49, 50, 50, 51, 51, 52, 52, 52, 53, 53, 52, 53, 53, 52,
    54, 53, 54, 54, 54, 53, 54, 54, 55, 53, 55, 54, 55, 53, 54, 54, 54, 53, 54,
    53, 53, 53, 54, 53, 53, 51, 53, 52, 52, 51, 51, 51, 51, 50, 50, 50, 49, 48,
    49, 49, 48, 48, 48, 47, 46, 46, 46, 45, 44, 43, 44, 43, 43, 42, 41, 42, 40,
    39, 40, 39, 38, 38, 38, 37, 35, 36, 35, 34, 34, 33, 33, 32, 32, 30, 31, 30,
    29, 28, 28, 27, 27, 25, 27, 25, 24, 23, 23, 23, 22, 21, 21, 21, 20, 19, 19,
    18, 17, 17, 16, 15, 15, 14, 15, 13, 13, 12, 11, 12, 11, 9,  10, 10, 9,  8,
    8,  8,  7,  6,  6,  6,  5,  5,  5,  5,  4,  3,  4,  4,  2,  2,  3,  2,  2,
    1,  2,  1,  1,  1,  1,  2,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1,
    2,  1,  2,  2,  2,  2,  4,  3,  3,  4,  5,  5,  5,  5,  7,  7,  7,  8,  9,
    9,  10, 11, 12, 12, 13, 14, 15, 16, 16, 17, 20, 19, 21, 21, 24, 24, 26, 26,
    29, 30, 31, 33, 34, 36, 36, 38, 40};

static const uint8_t kLogCorrection[] = {
  160, 32, 76, 160, 194, 32, 32, 76, 229, 160, 128, 194, 204, 32, 88, 32, 48,
  76, 66, 229, 16, 160, 121, 128, 159, 194, 215, 204, 148, 32, 101, 88, 239, 32,
  227, 48, 0, 76, 15, 66, 225, 229, 76, 16, 45, 160, 101, 121, 216, 128, 110,
  159, 18, 194, 175, 215, 54, 204, 150, 148, 194, 32, 172
};

static SinCosT makeSinCos() {
  SinCosT result;
  const int32_t kOne = result.kOne;
  const int32_t kMaxAngle = result.kMaxAngle;
  const int32_t kMaxAngle2 = kMaxAngle / 2;
  auto& kSin = result.kSin;
  auto& kCos = result.kCos;
  kSin[0] = 0;
  kCos[kMaxAngle2] = 0;
  for (int32_t i = 1; i < kMaxAngle2; ++i) {
    int32_t v =
        kSin[i - 1] + 1595 - i - (i / 4) - (i * i / 50) + kSinCorrection[i - 1];
    kSin[i] = v;
    kSin[kMaxAngle - i] = v;
    kCos[kMaxAngle2 - i] = v;
    kCos[kMaxAngle2 + i] = -v;
  }
  kSin[kMaxAngle2] = kOne;
  kCos[0] = kOne;

  for (int32_t i = 0; i < kMaxAngle; ++i) {
    int32_t c = kCos[i];
    double is = (i != 0) ? (1.0 / kSin[i]) : 0.0;
    result.kInvSin[i] = is;
    result.kMinusCot[i] = -c * is;
  }

  auto& kLog2 = result.kLog2;
  kLog2[0] = kLog2[1] = 0.0f;
  kLog2[2] = 1.0f;
  float m = 1.00135476f;
  float v = 2.0f;
  size_t j = 3;
  for (size_t i = 512; i <= 3084;) {
    v = v * m;
    i++;
    if (v >= j) {
      kLog2[j] = (i - (((int32_t)kLogCorrection[j - 3] - 32) / 256.0)) / 512.0;
      j++;
    }
  }

  return result;
}

const SinCosT SinCos = makeSinCos();

}  // namespace twim
