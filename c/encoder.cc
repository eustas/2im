#include "encoder.h"

#include "codec_params.h"
#include "distance_range.h"
#include "image.h"
#include "platform.h"

namespace twim {

class Cache;

class StatsCache {
 public:
  int32_t count;
  Owned<Vector<int32_t>> row_offset;
  Owned<Vector<float>> y;
  Owned<Vector<float>> x0;
  Owned<Vector<float>> x1;
  Owned<Vector<float>> x;

  int32_t plus[8];
  int32_t minus[8];

  template <typename T>
  static Owned<Vector<T>> alloc(size_t len) {
    VecTag<int32_t> d_i32;
    constexpr size_t n_i32 = d_i32.N;
    return allocVector<T>((len + n_i32 - 1) & ~(n_i32 - 1));
  }

  StatsCache(size_t capacity)
      : row_offset(alloc<int32_t>(capacity)),
        y(alloc<float>(capacity)),
        x0(alloc<float>(capacity)),
        x1(alloc<float>(capacity)),
        x(alloc<float>(capacity)) {}

  static void sum(Cache* cache, float* RESTRICT regionX, int32_t dst[8]) {
    StatsCache& stats_cache = cache->stats_cache;
    size_t count = stats_cache.count;
    int32_t* RESTRICT rowOffset = stats_cache.row_offset->data;
    const int32_t* RESTRICT sum_r = cache->uber->sum_r->data;
    const int32_t* RESTRICT sum_g = cache->uber->sum_g->data;
    const int32_t* RESTRICT sum_b = cache->uber->sum_b->data;
    const int32_t* RESTRICT sum_r2 = cache->uber->sum_r2->data;
    const int32_t* RESTRICT sum_g2 = cache->uber->sum_g2->data;
    const int32_t* RESTRICT sum_b2 = cache->uber->sum_b2->data;

    int32_t pixel_count = 0;
    int32_t r = 0;
    int32_t g = 0;
    int32_t b = 0;
    int32_t r2 = 0;
    int32_t g2 = 0;
    int32_t b2 = 0;
    for (size_t i = 0; i < count; i++) {
      int32_t x = (int32_t)regionX[i];
      int32_t offset = rowOffset[i] + x;
      pixel_count += x;
      r += sum_r[offset];
      g += sum_g[offset];
      b += sum_b[offset];
      r2 += sum_r2[offset];
      g2 += sum_g2[offset];
      b2 += sum_b2[offset];
    }

    dst[0] = pixel_count;
    dst[1] = r;
    dst[2] = g;
    dst[3] = b;
    dst[4] = r2;
    dst[5] = g2;
    dst[6] = b2;
  }

  void prepare(Cache* cache, Vector<int32_t>* region) {
    size_t count = region->len;
    size_t step = region->capacity / 3;
    size_t sum_stride = cache->uber->stride;
    int32_t* RESTRICT data = region->data;
    float* RESTRICT y = this->y->data;
    float* RESTRICT x0 = this->x0->data;
    float* RESTRICT x1 = this->x1->data;
    int32_t* RESTRICT row_offset = this->row_offset->data;
    for (size_t i = 0; i < count; ++i) {
      int32_t row = data[i];
      y[i] = row;
      x0[i] = data[step + i];
      x1[i] = data[2 * step + i];
      row_offset[i] = row * sum_stride;
    }
    this->count = count;
    sum(cache, x1, plus);
  }
};

class Stats {
 public:
  int32_t pixel_count;
  int32_t rgb[3];
  int32_t rgb2[3];

  void delta(Cache* cache, float* RESTRICT regionX) {
    StatsCache& stats_cache = cache->stats_cache;
    int32_t* RESTRICT minus = stats_cache.minus;
    StatsCache::sum(cache, regionX, minus);

    int32_t* RESTRICT plus = stats_cache.plus;
    pixel_count = plus[0] - minus[0];
    rgb[0] = plus[1] - minus[1];
    rgb[1] = plus[2] - minus[2];
    rgb[2] = plus[3] - minus[3];
    rgb2[0] = plus[4] - minus[4];
    rgb2[1] = plus[5] - minus[5];
    rgb2[2] = plus[6] - minus[6];
  }

  // Calculate stats from own region.
  void update(Cache* cache) { delta(cache, cache->stats_cache.x0->data); }

  void copy(const Stats& other) {
    pixel_count = other.pixel_count;
    rgb[0] = other.rgb[0];
    rgb[1] = other.rgb[1];
    rgb[2] = other.rgb[2];
    rgb2[0] = other.rgb2[0];
    rgb2[1] = other.rgb2[0];
    rgb2[2] = other.rgb2[0];
  }

  void sub(const Stats& other) {
    pixel_count -= other.pixel_count;
    rgb[0] -= other.rgb[0];
    rgb[1] -= other.rgb[1];
    rgb[2] -= other.rgb[2];
    rgb2[0] -= other.rgb2[0];
    rgb2[1] -= other.rgb2[0];
    rgb2[2] -= other.rgb2[0];
  }

  void update(const Stats& parent, const Stats& sibling) {
    copy(parent);
    sub(sibling);
  }

  // y * ny >= d
  static void updateGeHorizontal(Cache* cache, int32_t d) {
    int32_t ny = SinCos::kCos[0];
    float dny = d / (float)ny;
    StatsCache& stats_cache = cache->stats_cache;
    int32_t count = stats_cache.count;
    float* RESTRICT region_y = stats_cache.y->data;
    float* RESTRICT region_x0 = stats_cache.x0->data;
    float* RESTRICT region_x1 = stats_cache.x1->data;
    float* RESTRICT region_x = stats_cache.x->data;

    for (size_t i = 0; i < count; i++) {
      float y = region_y[i];
      float x0 = region_x0[i];
      float x1 = region_x1[i];
      region_x[i] = ((y - dny) < 0) ? x1 : x0;
    }
  }

  // x >= (d - y * ny) / nx
  static void updateGeGeneric(Cache* cache, int32_t angle, int32_t d) {
    int32_t nx = SinCos::kSin[angle];
    int32_t ny = SinCos::kCos[angle];
    float dnx = (2 * d + nx) / (float)(2 * nx);
    float mnynx = -(2 * ny) / (float)(2 * nx);
    StatsCache& stats_cache = cache->stats_cache;
    size_t count = stats_cache.count;
    float* RESTRICT region_y = stats_cache.y->data;
    float* RESTRICT region_x0 = stats_cache.x0->data;
    float* RESTRICT region_x1 = stats_cache.x1->data;
    float* RESTRICT region_x = stats_cache.x->data;

    for (size_t i = 0; i < count; i++) {
      float y = region_y[i];
      float xf = y * mnynx + dnx;  // FMA
      float x0 = region_x0[i];
      float x1 = region_x1[i];
      float m = ((xf - x0) > 0) ? xf : x0;
      region_x[i] = ((x1 - xf) > 0) ? m : x1;
    }
  }

  void updateGe(Cache* cache, int angle, int d) {
    if (angle == 0) {
      updateGeHorizontal(cache, d);
    } else {
      updateGeGeneric(cache, angle, d);
    }
    delta(cache, cache->stats_cache.x->data);
  }
};

class UberCache {
 public:
  const size_t height;
  const size_t stride;
  /* Cumulative sums. Extra column with total sum. */
  Owned<Vector<int32_t>> sum_r;
  Owned<Vector<int32_t>> sum_g;
  Owned<Vector<int32_t>> sum_b;
  Owned<Vector<int32_t>> sum_r2;
  Owned<Vector<int32_t>> sum_g2;
  Owned<Vector<int32_t>> sum_b2;

  UberCache(const Image& src)
      : height(src.height),
        stride(src.width + 1),
        sum_r(allocVector<int32_t>(stride * src.height)),
        sum_g(allocVector<int32_t>(stride * src.height)),
        sum_b(allocVector<int32_t>(stride * src.height)),
        sum_r2(allocVector<int32_t>(stride * src.height)),
        sum_g2(allocVector<int32_t>(stride * src.height)),
        sum_b2(allocVector<int32_t>(stride * src.height)) {
    int32_t* RESTRICT sum_r = this->sum_r->data;
    int32_t* RESTRICT sum_g = this->sum_g->data;
    int32_t* RESTRICT sum_b = this->sum_b->data;
    int32_t* RESTRICT sum_r2 = this->sum_r2->data;
    int32_t* RESTRICT sum_g2 = this->sum_g2->data;
    int32_t* RESTRICT sum_b2 = this->sum_b2->data;
    for (size_t y = 0; y < src.height; ++y) {
      size_t src_row_offset = y * src.width;
      const uint8_t* RESTRICT r_row = src.r.data() + src_row_offset;
      const uint8_t* RESTRICT g_row = src.g.data() + src_row_offset;
      const uint8_t* RESTRICT b_row = src.b.data() + src_row_offset;
      size_t dstRowOffset = y * stride;
      sum_r[dstRowOffset] = 0;
      sum_r2[dstRowOffset] = 0;
      sum_g[dstRowOffset] = 0;
      sum_g2[dstRowOffset] = 0;
      sum_b[dstRowOffset] = 0;
      sum_b2[dstRowOffset] = 0;
      for (int x = 0; x < src.width; ++x) {
        size_t dstOffset = dstRowOffset + x;
        int32_t r = r_row[x];
        int32_t g = g_row[x];
        int32_t b = b_row[x];
        sum_r[dstOffset + 1] = sum_r[dstOffset] + r;
        sum_r2[dstOffset + 1] = sum_r2[dstOffset] + r * r;
        sum_g[dstOffset + 1] = sum_g[dstOffset] + g;
        sum_g2[dstOffset + 1] = sum_g2[dstOffset] + g * g;
        sum_b[dstOffset + 1] = sum_b[dstOffset] + b;
        sum_b2[dstOffset + 1] = sum_b2[dstOffset] + b * b;
      }
    }
  }
};

class Cache {
 public:
  StatsCache stats_cache;
  const UberCache* uber;

  Stats stats[CodecParams::kMaxLineLimit + 3];

  Cache(const UberCache& uber) : stats_cache(uber.height) {}
};

static double score(const Stats& parent, const Stats& left, const Stats& right) {
  if (left.pixel_count == 0 || right.pixel_count == 0) {
    return 0.0;
  }

  double left_score[3];
  double right_score[3];
  for (size_t c = 0; c < 3; ++c) {
    double parent_average = (parent.rgb[c] + 0.0) / parent.pixel_count;
    double left_average = (left.rgb[c] + 0.0) / left.pixel_count;
    double right_average = (right.rgb[c] + 0.0) / right.pixel_count;
    double parent_average2 = parent_average * parent_average;
    double left_average2 = left_average * left_average;
    double right_average2 = right_average * right_average;
    left_score[c] = left.pixel_count * (parent_average2 - left_average2) -
                    2.0 * left.rgb[c] * (parent_average - left_average);
    right_score[c] = right.pixel_count * (parent_average2 - right_average2) -
                     2.0 * right.rgb[c] * (parent_average - right_average);
  }

  double sum_left = 0;
  double sum_right = 0;
  for (int c = 0; c < 3; ++c) {
    sum_left += left_score[c];
    sum_right += right_score[c];
  }

  return sum_left + sum_right;
}

class Fragment {
 public:
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

  Stats stats;

  // Subdivision.
  int32_t level;
  int32_t best_angle_code;
  int32_t best_line;
  double best_score;
  int32_t best_num_lines;
  double best_cost;

  Fragment(Owned<Vector<int32_t>> region) : region(std::move(region)) {}

  void encode(RangeEncoder* dst, const CodecParams& cp, bool is_leaf,
              std::vector<Fragment*>* children) {
    if (is_leaf) {
      RangeEncoder::writeNumber(dst, NodeType::COUNT, NodeType::FILL);
      for (size_t c = 0; c < 3; ++c) {
        int32_t q = cp.color_quant;
        int32_t d = 255 * stats.pixel_count;
        int32_t v = (2 * (q - 1) * stats.rgb[c] + d) / (2 * d);
        RangeEncoder::writeNumber(dst, q, v);
      }
      return;
    }
    RangeEncoder::writeNumber(dst, NodeType::COUNT, NodeType::HALF_PLANE);
    int32_t maxAngle = 1 << cp.angle_bits[level];
    RangeEncoder::writeNumber(dst, maxAngle, best_angle_code);
    RangeEncoder::writeNumber(dst, best_num_lines, best_line);
    children->push_back(left_child.get());
    children->push_back(right_child.get());
  }

  void simulateEncode(CodecParams cp, bool is_leaf,
                      std::vector<Fragment*>* children, double sqe[3]) {
    if (is_leaf) {
      for (size_t c = 0; c < 3; ++c) {
        int32_t q = cp.color_quant;
        int32_t d = 255 * stats.pixel_count;
        int32_t v = (2 * (q - 1) * stats.rgb[c] + d) / (2 * d);
        int32_t vq = CodecParams::dequantizeColor(v, q);
        // sum((vi - v)^2) == sum(vi^2 - 2 * vi * v + v^2)
        //                 == n * v^2 + sum(vi^2) - 2 * v * sum(vi)
        sqe[c] +=
            stats.pixel_count * vq * vq + stats.rgb2[c] - 2 * vq * stats.rgb[c];
      }
      return;
    }
    children->push_back(left_child.get());
    children->push_back(right_child.get());
  }

  void findBestSubdivision(Cache* cache, CodecParams cp) {
    Vector<int32_t>& region = *this->region.get();
    Stats& stats = this->stats;
    Stats* cache_stats = cache->stats;
    int32_t level = cp.getLevel(region);
    int32_t angle_max = 1 << cp.angle_bits[level];
    int32_t angle_mult = (SinCos::kMaxAngle / angle_max);
    int32_t best_angle_code = 0;
    int32_t best_line = 0;
    double best_score = -1.0;
    cache->stats_cache.prepare(cache, &region);
    stats.update(cache);

    // Find subdivision
    for (int32_t angle_code = 0; angle_code < angle_max; ++angle_code) {
      int32_t angle = angle_code * angle_mult;
      DistanceRange distance_range;
      distance_range.update(region, angle, cp);
      int32_t num_lines = distance_range.num_lines;
      cache_stats[0].setEmpty();
      for (int32_t line = 0; line < num_lines; ++line) {
        cache_stats[line + 1].updateGe(cache, angle,
                                       distance_range.distance(line));
      }
      cache_stats[num_lines + 1].copy(stats);

      for (int32_t line = 0; line < num_lines; ++line) {
        double full_score = 0.0;
        Stats tmp_stats3;
        tmp_stats3.update(cache_stats[line + 2], cache_stats[line]);
        Stats tmp_stats2;
        tmp_stats2.update(tmp_stats3, cache_stats[line + 1]);
        Stats tmp_stats1;
        tmp_stats1.update(tmp_stats3, tmp_stats2);
        full_score +=
            0.0 * score(tmp_stats3, tmp_stats1, tmp_stats2);

        tmp_stats1.update(stats, cache_stats[line + 1]);
        full_score += 1.0 * score(stats, cache_stats[line + 1], tmp_stats1);

        if (full_score > best_score) {
          best_angle_code = angle_code;
          best_line = line;
          best_score = full_score;
        }
      }
    }

    this->level = level;
    this->best_score = best_score;

    if (best_score < 0.0) {
      this->best_cost = -1.0;
    } else {
      DistanceRange distance_range;
      distance_range.update(region, best_angle_code * angle_mult, cp);
      int32_t child_step = Region.vectorFriendlySize(region[region.length - 1]);
      Owned<Vector<int32_t>> left_region = new int[child_step * 3 + 1];
      Owned<Vector<int32_t>>  right_region = new int[child_step * 3 + 1];
      Region.splitLine(region, best_angle_code * angle_mult,
                       distance_range.distance(best_line), left_region,
                       right_region);
      this->left_child = make_unique<Fragment>(std::move(left_region));
      this->right_child = make_unique<Fragment>(std::move(right_region));

      this->best_angle_code = best_angle_code;
      this->best_num_lines = distance_range.num_lines;
      this->best_line = best_line;
      this->best_cost = bitCost(NodeType::COUNT * angle_max *
                              distance_range.num_lines);
    }
  }
};

}  // namespace twim
