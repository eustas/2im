#include "codec_params.h"

#include <cmath>
#include <sstream>

#include "platform.h"
#include "sin_cos.h"

namespace twim {

void CodecParams::setColorCode(uint32_t code) {
  color_code = code;
  if (code < kNumColorQuantOptions) {
    color_quant = makeColorQuant(code);
    palette_size = 0;
  } else {
    color_quant = 0;
    palette_size = code - kNumColorQuantOptions + 1;
  }
}

/*constexpr*/ CodecParams::Params CodecParams::splitCode(uint32_t code) {
  Params result = {0, 0, 0, 0};
  result[0] = code % kMaxF1;
  code = code / kMaxF1;
  result[1] = code % kMaxF2;
  code = code / kMaxF2;
  result[2] = code % kMaxF3;
  code = code / kMaxF3;
  result[3] = code % kMaxF4;
  return result;
}

void CodecParams::setPartitionCode(uint32_t code) {
  setPartitionParams(splitCode(code));
}

void CodecParams::setPartitionParams(Params params) {
  this->params = params;
  uint32_t f1 = params[0];
  uint32_t f2 = params[1] + 2;
  uint32_t f3 = static_cast<uint32_t>(std::pow(10, 3 - params[2] / 5.0));
  uint32_t f4 = params[3];
  uint32_t scale = (width * width + height * height) * f2 * f2;
  for (size_t i = 0; i < kMaxLevel; ++i) {
    levelScale[i] = scale / kBaseScaleFactor;
    scale = (scale * kScaleStepFactor) / f3;
  }

  int32_t bits = SinCos::kMaxAngleBits - f1;
  for (uint32_t i = 0; i < kMaxLevel; ++i) {
    int32_t level_bits = bits - i - (i * f4) / 2;
    angle_bits[i] = level_bits > 0 ? (static_cast<uint32_t>(level_bits)) : 0;
  }
}

std::string CodecParams::toString() const {
  std::stringstream out;
  out << "p: " << params[0] << params[1] << params[2] << params[3]
      << ", l: " << line_limit << ", c: " << color_code;
  return out.str();
}

template <typename Decoder>
uint32_t readSize(Decoder* src) {
  uint32_t plus = 0;
  uint32_t bits = Decoder::readNumber(src, 8);
  do {
    plus = (plus + 1) << 3u;
    uint32_t extra = Decoder::readNumber(src, 8);
    bits = (bits << 3u) + extra;
  } while ((Decoder::readNumber(src, 2) == 1));
  return bits + plus;
}

CodecParams CodecParams::read(RangeDecoder* src) {
  uint32_t width = readSize(src);
  uint32_t height = readSize(src);
  CodecParams cp(width, height);
  Params params = {RangeDecoder::readNumber(src, kMaxF1),
                   RangeDecoder::readNumber(src, kMaxF2),
                   RangeDecoder::readNumber(src, kMaxF3),
                   RangeDecoder::readNumber(src, kMaxF4)};
  cp.setPartitionParams(params);
  cp.line_limit = RangeDecoder::readNumber(src, kMaxLineLimit) + 1;
  cp.setColorCode(RangeDecoder::readNumber(src, kMaxColorCode));
  return cp;
}

template <typename Encoder>
void writeSize(Encoder* dst, uint32_t value) {
  value -= 8;
  uint32_t chunks = 2;
  while (value > (1u << (chunks * 3))) {
    value -= (1u << (chunks * 3));
    chunks++;
  }
  for (uint32_t i = 0; i < chunks; ++i) {
    if (i > 1) {
      Encoder::writeNumber(dst, 2, 1);
    }
    Encoder::writeNumber(dst, 8, (value >> (3 * (chunks - i - 1))) & 7u);
  }
  Encoder::writeNumber(dst, 2, 0);
}

void CodecParams::write(RangeEncoder* dst) const {
  writeSize(dst, width);
  writeSize(dst, height);
  RangeEncoder::writeNumber(dst, kMaxF1, params[0]);
  RangeEncoder::writeNumber(dst, kMaxF2, params[1]);
  RangeEncoder::writeNumber(dst, kMaxF3, params[2]);
  RangeEncoder::writeNumber(dst, kMaxF4, params[3]);
  RangeEncoder::writeNumber(dst, kMaxLineLimit, line_limit - 1);
  RangeEncoder::writeNumber(dst, kMaxColorCode, color_code);
}

uint32_t CodecParams::getLevel(const Vector<int32_t>& region) const {
  size_t count = region.len;
  size_t step = region.capacity / 3;
  if (count == 0) {
    return kInvalid;
  }

  const int32_t* RESTRICT y = region.data();
  const int32_t* RESTRICT x0 = y + step;
  const int32_t* RESTRICT x1 = x0 + step;

  // TODO(eustas): perhaps box area is not the best value for level calculation.
  int32_t min_y = height + 1;
  int32_t max_y = -1;
  int32_t min_x = width + 1;
  int32_t max_x = -1;
  for (size_t i = 0; i < count; i++) {
    min_y = std::min(min_y, y[i]);
    max_y = std::max(max_y, y[i]);
    min_x = std::min(min_x, x0[i]);
    max_x = std::max(max_x, x1[i]);
  }

  int32_t dx = max_x - min_x;
  int32_t dy = max_y + 1 - min_y;
  uint32_t d = static_cast<uint32_t>(dx * dx + dy * dy);
  for (uint32_t i = 0; i < kMaxLevel; ++i) {
    if (d >= levelScale[i]) {
      return i;
    }
  }
  return kMaxLevel - 1;
}

}  // namespace twim
