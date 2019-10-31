#include "decoder.h"

#include <memory>
#include <vector>

#include "codec_params.h"
#include "distance_range.h"
#include "platform.h"
#include "range_decoder.h"
#include "region.h"
#include "sin_cos.h"

namespace twim {

namespace {

uint32_t readColor(RangeDecoder* src, const CodecParams& cp,
                   const uint32_t* palette) {
  if (cp.palette_size == 0) {
    uint32_t argb = 0xFF;  // alpha = 1
    for (size_t c = 0; c < 3; ++c) {
      uint32_t q = cp.color_quant;
      argb = (argb << 8u) |
             CodecParams::dequantizeColor(RangeDecoder::readNumber(src, q), q);
    }
    return argb;
  } else {
    return palette[RangeDecoder::readNumber(src, cp.palette_size)];
  }
}

class Fragment {
 public:
  int32_t type = NodeType::FILL;
  uint32_t color = 0;
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

  explicit Fragment(Owned<Vector<int32_t>> region)
      : region(std::move(region)) {}

  bool parse(RangeDecoder* src, const CodecParams& cp, uint32_t* palette,
             std::vector<Fragment*>* children) {
    type = RangeDecoder::readNumber(src, NodeType::COUNT);
    if (type == NodeType::FILL) {
      color = readColor(src, cp, palette);
      return true;
    }

    uint32_t level = cp.getLevel(*region.get());
    if (level == CodecParams::kInvalid) return false;  // corrupted input

    if (type != NodeType::HALF_PLANE) return false;

    uint32_t angleMax = 1u << cp.angle_bits[level];
    uint32_t angleMult = (SinCos::kMaxAngle / angleMax);
    uint32_t angleCode = RangeDecoder::readNumber(src, angleMax);
    uint32_t angle = angleCode * angleMult;
    DistanceRange distance_range(*region.get(), angle, cp);
    if (distance_range.num_lines == DistanceRange::kInvalid) return false;
    uint32_t line = RangeDecoder::readNumber(src, distance_range.num_lines);

    constexpr const auto vi32 = AvxVecTag<int32_t>();
    // Cutting with half-planes does not increase the number of scans.
    uint32_t step = vecSize(vi32, region->len);
    auto inner = allocVector(vi32, 3 * step);
    auto outer = allocVector(vi32, 3 * step);
    Region::splitLine(*region.get(), angle, distance_range.distance(line),
                      inner.get(), outer.get());
    left_child.reset(new Fragment(std::move(inner)));
    children->push_back(left_child.get());
    right_child.reset(new Fragment(std::move(outer)));
    children->push_back(right_child.get());
    return true;
  }

  void render(Image* out) {
    if (type == NodeType::FILL) {
      size_t width = out->width;
      uint8_t* RESTRICT r = out->r.data();
      uint8_t* RESTRICT g = out->g.data();
      uint8_t* RESTRICT b = out->b.data();
      size_t step = region->capacity / 3;
      size_t count = region->len;
      uint8_t cr = static_cast<uint8_t>(color & 0xFFu);
      uint8_t cg = static_cast<uint8_t>((color >> 8u) & 0xFFu);
      uint8_t cb = static_cast<uint8_t>((color >> 16u) & 0xFFu);
      const int32_t* RESTRICT vy = region->data();
      const int32_t* RESTRICT vx0 = vy + step;
      const int32_t* RESTRICT vx1 = vx0 + step;
      for (size_t i = 0; i < count; i++) {
        int32_t y = vy[i];
        int32_t x0 = vx0[i];
        int32_t x1 = vx1[i];
        for (int32_t x = x0; x < x1; ++x) {
          r[y * width + x] = cr;
          g[y * width + x] = cg;
          b[y * width + x] = cb;
        }
      }
      return;
    }
    left_child->render(out);
    right_child->render(out);
  }
};

}  // namespace

Image Decoder::decode(std::vector<uint8_t>&& encoded) {
  Image result = Image();

  RangeDecoder src(std::move(encoded));
  CodecParams cp = CodecParams::read(&src);
  uint32_t width = cp.width;
  uint32_t height = cp.height;

  std::vector<uint32_t> palette;
  for (size_t j = 0; j < cp.palette_size; ++j) {
    uint32_t argb = 0xFFu;  // alpha = 1
    for (size_t c = 0; c < 3; ++c) {
      argb = (argb << 8u) | RangeDecoder::readNumber(&src, 256);
    }
    palette.push_back(argb);
  }

  constexpr const auto vi32 = AvxVecTag<int32_t>();
  uint32_t step = vecSize(vi32, height);
  auto root_region = allocVector(vi32, 3 * step);
  int32_t* RESTRICT y = root_region->data();
  int32_t* RESTRICT x0 = y + step;
  int32_t* RESTRICT x1 = x0 + step;
  for (uint32_t i = 0; i < height; ++i) {
    y[i] = i;
    x0[i] = 0;
    x1[i] = width;
  }
  root_region->len = height;

  Fragment root(std::move(root_region));
  std::vector<Fragment*> children;
  children.push_back(&root);

  size_t cursor = 0;
  while (cursor < children.size()) {
    size_t checkpoint = children.size();
    for (; cursor < checkpoint; ++cursor) {
      bool ok = children[cursor]->parse(&src, cp, palette.data(), &children);
      if (!ok) {
        return result;
      }
    }
  }

  result.width = width;
  result.height = height;
  result.r.resize(width * height);
  result.g.resize(width * height);
  result.b.resize(width * height);
  root.render(&result);

  return result;
}

}  // namespace twim
