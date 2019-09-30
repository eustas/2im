#ifndef TWIM_CODEC_PARAMS
#define TWIM_CODEC_PARAMS

#include <string>

#include "platform.h"
#include "range_decoder.h"
#include "range_encoder.h"
#include "sin_cos.h"

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
  CodecParams(uint32_t width, uint32_t height) : width(width), height(height) {}
  static CodecParams read(RangeDecoder* src);
  void write(RangeEncoder* dst) const;

  constexpr int32_t getLineQuant() const { return SinCos::kOne; }

  static const uint32_t kInvalid = static_cast<uint32_t>(-1);
  uint32_t getLevel(const Vector<int32_t>& region) const;

  void setColorCode(uint32_t code);
  void setPartitionCode(uint32_t code);
  std::string toString() const;

  static constexpr const uint32_t kMaxLineLimit = 63;

  static constexpr const uint32_t kNumColorQuantOptions = 17;
  static constexpr const uint32_t kMaxColorPaletteSize = 32;
  static constexpr const uint32_t kMaxColorCode =
      kNumColorQuantOptions + kMaxColorPaletteSize;

  static constexpr uint32_t makeColorQuant(uint32_t code) {
    return 1u + ((4u + (code & 3u)) << (code >> 2u));
  }

  static constexpr uint32_t dequantizeColor(uint32_t v, uint32_t q) {
    return 255u * v / (q - 1u);
  }

 private:
  using Params = std::array<uint32_t, 4>;
  static constexpr const uint32_t kMaxLevel = 7;
  static constexpr const uint32_t kMaxF1 = 4;
  static constexpr const uint32_t kMaxF2 = 5;
  static constexpr const uint32_t kMaxF3 = 5;
  static constexpr const uint32_t kMaxF4 = 5;
  static constexpr const uint32_t kScaleStepFactor = 40;
  static constexpr const uint32_t kBaseScaleFactor = 36;

  static /*constexpr*/ Params splitCode(uint32_t code);
  void setPartitionParams(Params params);

  int32_t levelScale[kMaxLevel] = {0};

  Params params = Params();
  uint32_t color_code = 0;

 public:
  const uint32_t width;
  const uint32_t height;
  uint32_t color_quant = 0;
  uint32_t palette_size = 0;
  uint32_t line_limit = kMaxLineLimit;
  uint32_t angle_bits[kMaxLevel] = {0};
  static constexpr const int32_t kMaxPartitionCode =
      kMaxF1 * kMaxF2 * kMaxF3 * kMaxF4;
  static constexpr const int32_t kTax =
      kMaxPartitionCode * kMaxLineLimit * kMaxColorCode;
};

}  // namespace twim

#endif  // TWIM_CODEC_PARAMS
