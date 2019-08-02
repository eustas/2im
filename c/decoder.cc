#include "decoder.h"

#include <memory>
#include <vector>

#include "codec_params.h"
#include "distance_range.h"
#include "platform.h"
#include "range_decoder.h"
#include "region.h"
#include "sincos.h"

namespace twim {

namespace {

int32_t readColor(RangeDecoder* src, const CodecParams& cp) {
  int32_t argb = 0xFF;  // alpha = 1
  for (size_t c = 0; c < 3; ++c) {
    int32_t q = cp.color_quant;
    argb = (argb << 8) |
           CodecParams::dequantizeColor(RangeDecoder::readNumber(src, q), q);
  }
  return argb;
}

class Fragment {
 public:
  int32_t type = NodeType::FILL;
  int32_t color;
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

  Fragment(Owned<Vector<int32_t>> region) : region(std::move(region)) {}

  bool parse(RangeDecoder* src, const CodecParams& cp,
             std::vector<Fragment*>* children, DistanceRange* distanceRange) {
    type = RangeDecoder::readNumber(src, NodeType::COUNT);
    if (type == NodeType::FILL) {
      color = readColor(src, cp);
      return true;
    }

    int32_t level = cp.getLevel(*region.get());
    if (level < 0) return false;  // corrupted input

    if (type != NodeType::HALF_PLANE) return false;

    int32_t angleMax = 1 << cp.angle_bits[level];
    int32_t angleMult = (SinCos::kMaxAngle / angleMax);
    int32_t angleCode = RangeDecoder::readNumber(src, angleMax);
    int32_t angle = angleCode * angleMult;
    distanceRange->update(*region.get(), angle, cp);
    if (distanceRange->num_lines < 0) return false;
    int32_t line = RangeDecoder::readNumber(src, distanceRange->num_lines);

    // Cutting with half-planes does not increase the number of scans.
    int32_t step = vecSize<int32_t>(region->len);
    auto inner = allocVector<int32_t>(3 * step);
    auto outer = allocVector<int32_t>(3 * step);
    Region::splitLine(*region.get(), angle, distanceRange->distance(line),
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
      uint8_t cr = color & 0xFF;
      uint8_t cg = (color >> 8) & 0xFF;
      uint8_t cb = (color >> 16) & 0xFF;
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
  Image result;

  RangeDecoder src(std::move(encoded));
  CodecParams cp = CodecParams::read(&src);
  int32_t width = cp.width;
  int32_t height = cp.height;

  int32_t step = vecSize<int32_t>(height);
  auto root_region = allocVector<int32_t>(3 * step);
  int32_t* RESTRICT y = root_region->data();
  int32_t* RESTRICT x0 = y + step;
  int32_t* RESTRICT x1 = x0 + step;
  for (int32_t i = 0; i < height; ++i) {
    y[i] = i;
    x0[i] = 0;
    x1[i] = width;
  }
  root_region->len = height;

  Fragment root(std::move(root_region));
  std::vector<Fragment*> children;
  children.push_back(&root);

  DistanceRange distance_range;
  size_t cursor = 0;
  while (cursor < children.size()) {
    size_t checkpoint = children.size();
    for (; cursor < checkpoint; ++cursor) {
      bool ok = children[cursor]->parse(&src, cp, &children, &distance_range);
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
