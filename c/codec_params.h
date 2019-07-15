#ifndef TWIM_CODEC_PARAMS
#define TWIM_CODEC_PARAMS

#include <string>

#include "platform.h"
#include "range_decoder.h"
#include "range_encoder.h"
#include "sincos.h"

namespace twim {

class NodeType {
 public:
  enum {
    FILL = 0,
    HALF_PLANE = 1,

    COUNT = 2
  };
};

class CodecParams {
 public:
  CodecParams(int32_t width, int32_t height) : width(width), height(height) {}
  static CodecParams read(RangeDecoder* src);
  void write(RangeEncoder* dst) const;

  constexpr int32_t getLineQuant() const { return SinCos::kOne; }

  int32_t getLevel(const Vector<int32_t>& region) const;

  void setColorCode(int32_t code);
  void setPartitionCode(int32_t code);
  std::string toString() const;

  static constexpr const int32_t kMaxLineLimit = 63;
  static constexpr const int32_t kMaxColorCode = 17;

  static constexpr int32_t makeColorQuant(int32_t code) {
    return 1 + ((4 + (code & 3)) << (code >> 2));
  }

  static constexpr int32_t dequantizeColor(int32_t v, int32_t q) {
    return (255 * v + q - 2) / (q - 1);
  }

 private:
  using Params = std::array<int32_t, 4>;
  static constexpr const int32_t kMaxLevel = 7;
  static constexpr const int32_t kMaxF1 = 4;
  static constexpr const int32_t kMaxF2 = 5;
  static constexpr const int32_t kMaxF3 = 5;
  static constexpr const int32_t kMaxF4 = 5;
  static constexpr const int32_t kScaleStepFactor = 40;
  static constexpr const int32_t kBaseScaleFactor = 36;

  static /*constexpr*/ Params splitCode(int32_t code);
  void setPartitionParams(Params params);

  int32_t levelScale[kMaxLevel];

  Params params;
  int32_t colorCode;

 public:
  const int32_t width;
  const int32_t height;
  int32_t colorQuant;
  int32_t lineLimit = kMaxLineLimit;
  int32_t angleBits[kMaxLevel];
  static constexpr const int32_t kMaxPartitionCode =
      kMaxF1 * kMaxF2 * kMaxF3 * kMaxF4;
  static constexpr const int32_t kTax =
      kMaxPartitionCode * kMaxLineLimit * kMaxColorCode;
};

}  // namespace twim

#endif  // TWIM_CODEC_PARAMS
