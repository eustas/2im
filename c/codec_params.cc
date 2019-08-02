#include "codec_params.h"

#include <cmath>
#include <sstream>

#include "platform.h"
#include "sincos.h"

namespace twim {

void CodecParams::setColorCode(int32_t code) {
  color_code = code;
  color_quant = makeColorQuant(code);
}

/*constexpr*/ CodecParams::Params CodecParams::splitCode(int32_t code) {
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

void CodecParams::setPartitionCode(int32_t code) {
  setPartitionParams(splitCode(code));
}

void CodecParams::setPartitionParams(Params params) {
  this->params = params;
  int32_t f1 = params[0];
  int32_t f2 = params[1] + 2;
  int32_t f3 = (int32_t)std::pow(10, 3 - params[2] / 5.0);
  int32_t f4 = params[3];
  int32_t scale = (width * width + height * height) * f2 * f2;
  for (size_t i = 0; i < kMaxLevel; ++i) {
    levelScale[i] = scale / kBaseScaleFactor;
    scale = (scale * kScaleStepFactor) / f3;
  }

  int32_t bits = SinCos::kMaxAngleBits - f1;
  for (int32_t i = 0; i < kMaxLevel; ++i) {
    angle_bits[i] = std::max(bits - i - (i * f4) / 2, 0);
  }
}

std::string CodecParams::toString() const {
  std::stringstream out;
  out << "p: " << params[0] << params[1] << params[2] << params[3]
      << ", l: " << line_limit << ", c: " << color_code;
  return out.str();
}

CodecParams CodecParams::read(RangeDecoder* src) {
  int32_t width = RangeDecoder::readSize(src);
  int32_t height = RangeDecoder::readSize(src);
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

void CodecParams::write(RangeEncoder* dst) const {
  RangeEncoder::writeSize(dst, width);
  RangeEncoder::writeSize(dst, height);
  RangeEncoder::writeNumber(dst, kMaxF1, params[0]);
  RangeEncoder::writeNumber(dst, kMaxF2, params[1]);
  RangeEncoder::writeNumber(dst, kMaxF3, params[2]);
  RangeEncoder::writeNumber(dst, kMaxF4, params[3]);
  RangeEncoder::writeNumber(dst, kMaxLineLimit, line_limit - 1);
  RangeEncoder::writeNumber(dst, kMaxColorCode, color_code);
}

int32_t CodecParams::getLevel(const Vector<int32_t>& region) const {
  size_t count = region.len;
  size_t step = region.capacity / 3;
  if (count == 0) {
    return -1;
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
  int32_t d = dx * dx + dy * dy;
  for (int32_t i = 0; i < kMaxLevel; ++i) {
    if (d >= levelScale[i]) {
      return i;
    }
  }
  return kMaxLevel - 1;
}

}  // namespace twim
