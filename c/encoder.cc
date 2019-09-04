#include "encoder.h"

#include <cmath>
#include <future>
#include <list>
#include <queue>
#include <vector>

#include "codec_params.h"
#include "distance_range.h"
#include "image.h"
#include "platform.h"
#include "region.h"

namespace twim {

namespace {

class Cache;

constexpr auto kVHD = SseVecTag<double>();
static_assert(kVHD.N == 2, "Expected vector size is 2");

constexpr auto kVHF = SseVecTag<float>();
static_assert(kVHF.N == 4, "Expected vector size is 4");

constexpr auto kVHI32 = SseVecTag<int32_t>();
static_assert(kVHI32.N == 4, "Expected vector size is 4");

constexpr auto kVF = AvxVecTag<float>();
static_assert(kVF.N == 8, "Expected vector size is 8");

constexpr auto kVD = AvxVecTag<double>();
static_assert(kVD.N == 4, "Expected vector size is 4");

constexpr auto kVI32 = AvxVecTag<int32_t>();
static_assert(kVI32.N == 8, "Expected vector size is 8");

constexpr const size_t kStride = kVHF.N;

class Stats {
 public:
  // r, g, b, pixel_count
  ALIGNED_16 float values[4];

  INLINE float rgb(size_t c) const { return values[c]; }
  INLINE float pixelCount() const { return values[3]; }

  SIMD INLINE void reset() { store(zero(kVHF), kVHF, values); }

  SIMD INLINE void diff(const Stats& plus, const Stats& minus) {
    const auto p = load(kVHF, plus.values);
    const auto m = load(kVHF, minus.values);
    store(sub(kVHF, p, m), kVHF, values);
  }

  SIMD INLINE void copy(const Stats& other) {
    store(load(kVHF, other.values), kVHF, values);
  }

  // y * ny >= d
  static void updateGeHorizontal(Cache* cache, int32_t d);

  // x >= (d - y * ny) / nx
  static void updateGeGeneric(Cache* cache, int32_t angle, int32_t d);

  void updateGe(Cache* cache, int angle, int d);
};

class StatsCache {
 public:
  Stats plus;
  Stats minus;

  int32_t count;
  Owned<Vector<int32_t>> row_offset;
  Owned<Vector<float>> y;
  Owned<Vector<int32_t>> x0;
  Owned<Vector<int32_t>> x1;
  Owned<Vector<int32_t>> x;

  StatsCache(size_t capacity)
      : row_offset(allocVector(kVI32, capacity)),
        y(allocVector(kVF, capacity)),
        x0(allocVector(kVI32, capacity)),
        x1(allocVector(kVI32, capacity)),
        x(allocVector(kVI32, capacity)) {}

  template <bool ABS>
  static void sum(Cache* cache, int32_t* RESTRICT region_x, Stats* dst);

  void prepare(Cache* cache, Vector<int32_t>* region);
};

class UberCache {
 public:
  const size_t width;
  const size_t height;
  const size_t stride;
  /* Cumulative sums. Extra column with total sum. */
  Owned<Vector<float>> sum;

  float rgb2[3] = {0.0f};

  UberCache(const Image& src)
      : width(src.width),
        height(src.height),
        stride(4 * (src.width + 1)),
        sum(allocVector(kVHF, stride * src.height)) {
    float* RESTRICT sum = this->sum->data();
    for (size_t y = 0; y < src.height; ++y) {
      float row_rgb2[3] = {0.0f};
      size_t src_row_offset = y * src.width;
      const uint8_t* RESTRICT r_row = src.r.data() + src_row_offset;
      const uint8_t* RESTRICT g_row = src.g.data() + src_row_offset;
      const uint8_t* RESTRICT b_row = src.b.data() + src_row_offset;
      size_t dstRowOffset = y * stride;
      for (size_t i = 0; i < 4; ++i) sum[dstRowOffset + i] = 0.0f;
      for (size_t x = 0; x < src.width; ++x) {
        size_t dstOffset = dstRowOffset + 4 * x;
        float r = r_row[x];
        float g = g_row[x];
        float b = b_row[x];
        sum[dstOffset + 4] = sum[dstOffset + 0] + r;
        sum[dstOffset + 5] = sum[dstOffset + 1] + g;
        sum[dstOffset + 6] = sum[dstOffset + 2] + b;
        sum[dstOffset + 7] = sum[dstOffset + 3] + 1.0f;
        row_rgb2[0] += r * r;
        row_rgb2[1] += g * g;
        row_rgb2[2] += b * b;
      }
      for (size_t c = 0; c < 3; ++c) rgb2[c] += row_rgb2[c];
    }
  }
};

class Cache {
 public:
  const UberCache* uber;
  StatsCache stats_cache;

  Stats stats[CodecParams::kMaxLineLimit + 3];

  Cache(const UberCache& uber) : uber(&uber), stats_cache(uber.height) {}
};

template <bool ABS>
void INLINE SIMD StatsCache::sum(Cache* cache, int32_t* RESTRICT region_x,
                                 Stats* dst) {
  StatsCache& stats_cache = cache->stats_cache;
  size_t count = stats_cache.count;
  const int32_t* RESTRICT row_offset = stats_cache.row_offset->data();
  const float* RESTRICT sum = cache->uber->sum->data();

  constexpr auto vhf = SseVecTag<float>();
  auto tmp = zero(vhf);
  for (size_t i = 0; i < count; i += kStride) {
    for (size_t j = 0; j < kStride; ++j) {
      int32_t offset =
          ABS ? region_x[i + j] : (row_offset[i + j] + 4 * region_x[i + j]);
      tmp = add(vhf, tmp, load(vhf, sum + offset));
    }
  }
  store(tmp, vhf, dst->values);
}

void INLINE SIMD StatsCache::prepare(Cache* cache, Vector<int32_t>* region) {
  size_t count = region->len;
  size_t step = region->capacity / 3;
  size_t sum_stride = cache->uber->stride;
  int32_t* RESTRICT data = region->data();
  float* RESTRICT y = this->y->data();
  int32_t* RESTRICT x0 = this->x0->data();
  int32_t* RESTRICT x1 = this->x1->data();
  int32_t* RESTRICT row_offset = this->row_offset->data();
  for (size_t i = 0; i < count; ++i) {
    int32_t row = data[i];
    y[i] = row;
    x0[i] = data[step + i];
    x1[i] = data[2 * step + i];
    row_offset[i] = row * sum_stride;
  }
  while ((count & (kStride - 1)) != 0) {
    y[count] = 0;
    x0[count] = 0;
    x1[count] = 0;
    row_offset[count] = 0;
    count++;
  }
  this->count = count;
}

void INLINE Stats::updateGeHorizontal(Cache* cache, int32_t d) {
  int32_t ny = SinCos::kCos[0];
  float dny = d / (float)ny;  // TODO: precalculate
  StatsCache& stats_cache = cache->stats_cache;
  size_t count = stats_cache.count;
  int32_t* RESTRICT row_offset = stats_cache.row_offset->data();
  float* RESTRICT region_y = stats_cache.y->data();
  int32_t* RESTRICT region_x0 = stats_cache.x0->data();
  int32_t* RESTRICT region_x1 = stats_cache.x1->data();
  int32_t* RESTRICT region_x = stats_cache.x->data();

  for (size_t i = 0; i < count; i++) {
    float y = region_y[i];
    int32_t offset = row_offset[i];
    int32_t x0 = region_x0[i];
    int32_t x1 = region_x1[i];
    int32_t x = (y < dny) ? x1 : x0;
    int32_t x_off = 4 * x + offset;
    region_x[i] = x_off;
  }
}

SIMD INLINE void Stats::updateGeGeneric(Cache* cache, int32_t angle,
                                        int32_t d) {
  int32_t nx = SinCos::kSin[angle];
  int32_t ny = SinCos::kCos[angle];
  float dnx = (2 * d + nx) / (float)(2 * nx);
  float mnynx = -(2 * ny) / (float)(2 * nx);
  StatsCache& stats_cache = cache->stats_cache;
  int32_t* RESTRICT row_offset = stats_cache.row_offset->data();
  float* RESTRICT region_y = stats_cache.y->data();
  int32_t* RESTRICT region_x0 = stats_cache.x0->data();
  int32_t* RESTRICT region_x1 = stats_cache.x1->data();
  int32_t* RESTRICT region_x = stats_cache.x->data();

  const auto& vf = kVHF;
  const auto& vi32 = kVHI32;
  const auto k4 = set1(vi32, 4);
  const auto dnx_ = set1(vf, dnx);
  const auto mnynx_ = set1(vf, mnynx);
  const size_t count = stats_cache.count;
  for (size_t i = 0; i < count; i += vf.N) {
    const auto y = load(vf, region_y + i);
    const auto offset = load(vi32, row_offset + i);
    const auto xf = add(vf, mul(vf, y, mnynx_), dnx_);
    const auto xi = cast(vf, vi32, xf);
    const auto x0 = load(vi32, region_x0 + i);
    const auto x1 = load(vi32, region_x1 + i);
    const auto x = min(vi32, x1, max(vi32, xi, x0));
    const auto x_off = add(vi32, mul(vi32, k4, x), offset);
    store(x_off, vi32, region_x + i);
  }
}

SIMD INLINE void Stats::updateGe(Cache* cache, int angle, int d) {
  if (angle == 0) {
    updateGeHorizontal(cache, d);
  } else {
    updateGeGeneric(cache, angle, d);
  }
}

static SIMD INLINE float reduce(const __m128 a_b_c_d) {
  const auto b_b_d_d = _mm_movehdup_ps(a_b_c_d);
  const auto ab_x_cd_x = _mm_add_ps(a_b_c_d, b_b_d_d);
  const auto cd_x_x_x = _mm_movehl_ps(b_b_d_d, ab_x_cd_x);
  const auto abcd_x_x_x = _mm_add_ss(ab_x_cd_x, cd_x_x_x);
  return _mm_cvtss_f32(abcd_x_x_x);
}

static SIMD INLINE float score(const Stats& whole, const Stats& left,
                               const Stats& right) {
  if ((left.pixelCount() <= 0.0f) || (right.pixelCount() <= 0.0f)) return 0.0f;

  const auto k1 = set1(kVHF, 1.0f);
  const auto pc = _mm_set_ps(1.0f, right.pixelCount(), left.pixelCount(),whole.pixelCount());
  const auto inv_pc = div(kVHF, k1, pc);

  const auto k2 = set1(kVHF, 2.0f);
  const auto whole_values = load(kVHF, whole.values);
  const auto whole_average = mul(kVHF, whole_values, broadcast<0>(kVHF, inv_pc));

  const auto left_values = load(kVHF, left.values);
  const auto left_pixel_count = set1(kVHF, left.pixelCount());
  const auto left_average = mul(kVHF, left_values, broadcast<1>(kVHF, inv_pc));
  const auto left_plus = add(kVHF, whole_average, left_average);
  const auto left_minus = sub(kVHF, whole_average, left_average);
  const auto left_a = mul(kVHF, left_pixel_count, left_plus);
  const auto left_b = mul(kVHF, k2, left_values);
  const auto left_sum = mul(kVHF, left_minus, sub(kVHF, left_a, left_b));

  const auto right_values = load(kVHF, right.values);
  const auto right_pixel_count = set1(kVHF, right.pixelCount());
  const auto right_average = mul(kVHF, right_values, broadcast<2>(kVHF, inv_pc));
  const auto right_plus = add(kVHF, whole_average, right_average);
  const auto right_minus = sub(kVHF, whole_average, right_average);
  const auto right_a = mul(kVHF, right_pixel_count, right_plus);
  const auto right_b = mul(kVHF, k2, right_values);
  const auto right_sum = mul(kVHF, right_minus, sub(kVHF, right_a, right_b));

  return reduce(add(kVHF, left_sum, right_sum));
}

static float bitCost(int32_t range) {
  static float kInvLog2 = 1.0f / std::log(2.0f);
  return std::log(range) * kInvLog2;
}

static int32_t sizeCost(int32_t value) {
  if (value < 8) return -1;
  value -= 8;
  int32_t bits = 6;
  while (value > (1 << bits)) {
    value -= (1 << bits);
    bits += 3;
  }
  return bits + (bits / 3) - 1;
}

SIMD INLINE static void chooseColor(const __m128 rgb0,
                                    const float* RESTRICT palette,
                                    size_t palette_size, size_t* RESTRICT index,
                                    float* RESTRICT distance2) {
  const size_t m = palette_size;
  size_t best_j = 0;
  float best_d2 = 1e35f;
  for (size_t j = 0; j < m; ++j) {
    const auto center = load(kVHF, palette + 4 * j);
    const auto d = sub(kVHF, rgb0, center);
    float d2 = reduce(mul(kVHF, d, d));
    if (d2 < best_d2) {
      best_j = j;
      best_d2 = d2;
    }
  }
  *index = best_j;
  *distance2 = best_d2;
}

class Fragment {
 public:
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

  Stats stats;
  Stats stats2;

  // Subdivision.
  int32_t ordinal = 0x7FFFFFFF;
  int32_t level;
  int32_t best_angle_code;
  int32_t best_line;
  float best_score;
  int32_t best_num_lines;
  float best_cost;

  Fragment(Fragment&&) = default;
  Fragment& operator=(Fragment&&) = default;
  Fragment(const Fragment&) = delete;
  Fragment& operator=(const Fragment&) = delete;

  explicit Fragment(Owned<Vector<int32_t>>&& region)
      : region(std::move(region)) {}

  SIMD NOINLINE void encode(RangeEncoder* dst, const CodecParams& cp,
                            bool is_leaf, const float* RESTRICT palette,
                            std::vector<Fragment*>* children) {
    if (is_leaf) {
      RangeEncoder::writeNumber(dst, NodeType::COUNT, NodeType::FILL);
      if (cp.palette_size == 0) {
        float quant = (cp.color_quant - 1) / 255.0f;
        for (size_t c = 0; c < 3; ++c) {
          int32_t v = static_cast<int32_t>(
              quant * stats.rgb(c) / stats.pixelCount() + 0.5f);
          RangeEncoder::writeNumber(dst, cp.color_quant, v);
        }
      } else {
        size_t index;
        float dummy;
        const auto sum_rgbc = load(kVHF, stats.values);
        const auto pixel_count = broadcast<3>(kVHF, sum_rgbc);
        const auto rgb1 = div(kVHF, sum_rgbc, pixel_count);
        // It is not necessary, but let's be consistent.
        const auto rgb0 = blend<0x8>(kVHF, rgb1, zero(kVHF));
        chooseColor(rgb0, palette, cp.palette_size, &index, &dummy);
        RangeEncoder::writeNumber(dst, cp.palette_size, index);
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

  void SIMD INLINE findBestSubdivision(Cache* cache, CodecParams cp) {
    Vector<int32_t>& region = *this->region.get();
    Stats& stats = this->stats;
    Stats* cache_stats = cache->stats;
    int32_t level = cp.getLevel(region);
    int32_t angle_max = 1 << cp.angle_bits[level];
    int32_t angle_mult = (SinCos::kMaxAngle / angle_max);
    int32_t best_angle_code = 0;
    int32_t best_line = 0;
    float best_score = -1.0f;
    StatsCache& stats_cache = cache->stats_cache;
    stats_cache.prepare(cache, &region);
    StatsCache::sum<false>(cache, stats_cache.x1->data(), &stats_cache.plus);
    StatsCache::sum<false>(cache, stats_cache.x0->data(), &stats_cache.minus);
    stats.diff(stats_cache.plus, stats_cache.minus);

    // Find subdivision
    for (int32_t angle_code = 0; angle_code < angle_max; ++angle_code) {
      int32_t angle = angle_code * angle_mult;
      DistanceRange distance_range;
      distance_range.update(region, angle, cp);
      int32_t num_lines = distance_range.num_lines;
      cache_stats[0].reset();
      for (int32_t line = 0; line < num_lines; ++line) {
        cache_stats[line + 1].updateGe(cache, angle,
                                       distance_range.distance(line));
        StatsCache::sum<true>(cache, stats_cache.x->data(), &stats_cache.minus);
        cache_stats[line + 1].diff(stats_cache.plus, stats_cache.minus);
      }
      cache_stats[num_lines + 1].copy(stats);

      for (int32_t line = 0; line < num_lines; ++line) {
        float full_score = 0.0f;
        constexpr const float kLocalWeight = 0.0f;
        if (kLocalWeight > 0.0f) {
          Stats band;
          band.diff(cache_stats[line + 2], cache_stats[line]);
          Stats left;
          left.diff(band, cache_stats[line + 1]);
          Stats right;
          right.diff(band, left);
          full_score += kLocalWeight * (score(band, left, right));
        }

        {
          Stats right;
          right.diff(stats, cache_stats[line + 1]);
          full_score += score(stats, cache_stats[line + 1], right);
        }

        if (full_score > best_score) {
          best_angle_code = angle_code;
          best_line = line;
          best_score = full_score;
        }
      }
    }

    this->level = level;
    this->best_score = best_score;

    if (best_score < 0.0f) {
      this->best_cost = -1.0f;
    } else {
      DistanceRange distance_range;
      distance_range.update(region, best_angle_code * angle_mult, cp);
      int32_t child_step = vecSize(kVI32, region.len);
      this->left_child.reset(new Fragment(allocVector(kVI32, 3 * child_step)));
      this->right_child.reset(new Fragment(allocVector(kVI32, 3 * child_step)));
      Region::splitLine(region, best_angle_code * angle_mult,
                        distance_range.distance(best_line),
                        this->left_child->region.get(),
                        this->right_child->region.get());

      this->best_angle_code = best_angle_code;
      this->best_num_lines = distance_range.num_lines;
      this->best_line = best_line;
      this->best_cost =
          bitCost(NodeType::COUNT * angle_max * distance_range.num_lines);
    }
  }
};

struct FragmentComparator {
  bool operator()(const Fragment* left, const Fragment* right) const {
    return left->best_score < right->best_score;
  }
};

static Fragment makeRoot(int32_t width, int32_t height) {
  constexpr const auto vi32 = AvxVecTag<int32_t>();
  int32_t step = vecSize(vi32, height);
  Owned<Vector<int32_t>> root = allocVector(vi32, 3 * step);
  int32_t* RESTRICT data = root->data();
  for (int32_t y = 0; y < height; ++y) {
    data[y] = y;
    data[step + y] = 0;
    data[2 * step + y] = width;
  }
  root->len = height;
  return Fragment(std::move(root));
}

/**
 * Builds the space partition.
 *
 * Minimal color data cost is used.
 * Partition could be used to try multiple color quantizations to see, which one
 * gives the best result.
 */
static SIMD NOINLINE std::vector<Fragment*> buildPartition(
    Fragment* root, size_t size_limit, const CodecParams& cp, Cache* cache) {
  float tax = bitCost(NodeType::COUNT);
  float budget = size_limit * 8.0f - tax -
                 bitCost(CodecParams::kTax);  // Minus flat image cost.

  std::vector<Fragment*> result;
  std::priority_queue<Fragment*, std::vector<Fragment*>, FragmentComparator>
      queue;
  root->findBestSubdivision(cache, cp);
  queue.push(root);
  while (!queue.empty()) {
    Fragment* candidate = queue.top();
    queue.pop();
    if (candidate->best_score < 0.0 || candidate->best_cost < 0.0) break;
    if (tax + candidate->best_cost <= budget) {
      budget -= tax + candidate->best_cost;
      candidate->ordinal = result.size();
      result.push_back(candidate);
      candidate->left_child->findBestSubdivision(cache, cp);
      queue.push(candidate->left_child.get());
      candidate->right_child->findBestSubdivision(cache, cp);
      queue.push(candidate->right_child.get());
    }
  }
  return result;
}

/* Returns the index of the first leaf node in partition. */
static INLINE size_t subpartition(int32_t target_size,
                                  const std::vector<Fragment*>& partition,
                                  CodecParams cp) {
  float node_tax = bitCost(NodeType::COUNT);
  float image_tax =
      bitCost(CodecParams::kTax) + sizeCost(cp.width) + sizeCost(cp.height);
  if (cp.palette_size == 0) {
    node_tax += 3.0f * bitCost(cp.color_quant);
  } else {
    node_tax += bitCost(cp.palette_size);
    image_tax += cp.palette_size * 3.0f * 8.0f;
  }
  float budget = 8.0f * target_size - image_tax + node_tax;
  size_t i;
  for (i = 0; i < partition.size(); ++i) {
    Fragment* node = partition[i];
    if (node->best_cost < 0.0f) break;
    float cost = node->best_cost + node_tax;
    if (budget < cost) break;
    budget -= cost;
  }
  return i;
}

/*
 * https://xkcd.com/221/
 *
 * Chosen by fair dice roll. Guaranteed to be random.
 */
static float kRandom[64] = {
    0.196761043301f, 0.735187971367f, 0.240889315713f, 0.322607869281f,
    0.042645685482f, 0.100931237742f, 0.436030039105f, 0.949386184145f,
    0.391385426476f, 0.693787004045f, 0.957348258524f, 0.401165324581f,
    0.200612435526f, 0.774156296613f, 0.702329904898f, 0.794307087431f,
    0.913297998115f, 0.418554329021f, 0.305256297017f, 0.447050968372f,
    0.275901615626f, 0.423281943403f, 0.222083232637f, 0.259557717968f,
    0.134780340092f, 0.624659579778f, 0.459554804245f, 0.629270576873f,
    0.728531198516f, 0.270863443275f, 0.730693946899f, 0.958482839910f,
    0.597229071250f, 0.020570415805f, 0.876483717523f, 0.734546837759f,
    0.548824137588f, 0.628979430442f, 0.813059781398f, 0.145038174534f,
    0.174453058546f, 0.195531684379f, 0.127489363209f, 0.878269052109f,
    0.990909412408f, 0.109277869329f, 0.295625366456f, 0.247012273577f,
    0.508121083167f, 0.875779718247f, 0.863389034205f, 0.663539415689f,
    0.069178093049f, 0.859564180486f, 0.560775815455f, 0.039552534504f,
    0.989061239776f, 0.815374917179f, 0.951061519055f, 0.211362121050f,
    0.255234747636f, 0.047947586972f, 0.520984579718f, 0.399461090480f};

SIMD INLINE static void makePalette(float* RESTRICT storage, size_t num_patches,
                                    size_t palette_size) {
  const size_t n = num_patches;
  const size_t m = palette_size;
  const float* RESTRICT stats = storage;
  float* RESTRICT centers = storage + 4 * n;
  float* RESTRICT centers_acc = centers + 4 * m;
  float* RESTRICT weights = centers_acc + 4 * m;
  const auto k0 = zero(kVHF);

  {
    // Choose one center uniformly at random from among the data points.
    float total = 0.0f;
    size_t i;
    for (i = 0; i < n; ++i) total += stats[4 * i + 3];
    float target = total * kRandom[0];
    float partial = 0.0f;
    for (i = 0; i < n; ++i) {
      partial += stats[4 * i + 3];
      if (partial >= target) break;
    }
    i = std::max(i, n - 1);
    const auto rgbc = load(kVHF, stats + 4 * i);
    const auto rgb0 = blend<0x8>(kVHF, rgbc, k0);
    store(rgb0, kVHF, centers);
  }

  for (size_t j = 1; j < m; ++j) {
    // D(x) is the distance to the nearest center.
    // Choose next with probability proportional to D(x)^2.
    size_t i;
    size_t total = 0.0f;
    for (i = 0; i < n; ++i) {
      const auto rgbc = load(kVHF, stats + 4 * i);
      const auto rgb0 = blend<0x8>(kVHF, rgbc, k0);
      float best_distance = 1e35f;
      for (size_t k = 0; k < j; ++k) {
        const auto center = load(kVHF, centers + 4 * k);
        const auto d = sub(kVHF, rgb0, center);
        float distance = reduce(mul(kVHF, d, d));
        if (distance < best_distance) best_distance = distance;
      }
      float weight = best_distance * stats[4 * i + 3];
      weights[i] = weight;
      total += weight;
    }
    float target = total * kRandom[j];
    float partial = 0;
    for (i = 0; i < n; ++i) {
      partial += weights[i];
      if (partial >= target) break;
    }
    i = std::max(i, n - 1);
    const auto rgbc = load(kVHF, stats + 4 * i);
    const auto rgb0 = blend<0x8>(kVHF, rgbc, k0);
    store(rgb0, kVHF, centers + 4 * j);
  }

  float last_score = 1e35f;
  while (true) {
    for (size_t j = 0; j < m; ++j) store(k0, kVHF, centers_acc + 4 * j);
    float score = 0.0f;
    for (size_t i = 0; i < n; ++i) {
      const auto rgbc = load(kVHF, stats + 4 * i);
      const auto rgb0 = blend<0x8>(kVHF, rgbc, k0);
      size_t index;
      float d2;
      chooseColor(rgb0, centers, m, &index, &d2);
      const float pixel_count_value = stats[4 * i + 3];
      const auto pixel_count = set1(kVHF, pixel_count_value);
      score += d2 * pixel_count_value;
      const auto weighted_rgb0 = mul(kVHF, rgb0, pixel_count);
      const auto weighted_rgbc = blend<0x8>(kVHF, weighted_rgb0, pixel_count);
      auto center_acc = load(kVHF, centers_acc + 4 * index);
      center_acc = add(kVHF, center_acc, weighted_rgbc);
      store(center_acc, kVHF, centers_acc + 4 * index);
    }
    for (size_t j = 0; j < m; ++j) {
      const float pixel_count_value = centers_acc[4 * j + 3];
      const auto pixel_count = set1(kVHF, pixel_count_value);
      if (pixel_count_value < 0.5f) {
        // Ooops, an orphaned center... Let it be.
      } else {
        const auto weighted_rgbc = load(kVHF, centers_acc + 4 * j);
        const auto rgb1 = div(kVHF, weighted_rgbc, pixel_count);
        const auto rgb0 = blend<0x8>(kVHF, rgb1, k0);
        store(rgb0, kVHF, centers + 4 * j);
      }
    }
    if (last_score - score < 1.0f) break;
    last_score = score;
  }
}

SIMD NOINLINE static Owned<Vector<float>> gatherPatchesAndBuildPalette(
    const std::vector<Fragment*>& partition, size_t num_non_leaf,
    size_t palette_size) {
  size_t n = num_non_leaf + 1;
  size_t m = palette_size;
  size_t stats_size = 4 * n;
  size_t palette_space = 4 * m;
  size_t extra_space = 4 * m + ((m > 0) ? n : 0);
  Owned<Vector<float>> result =
      allocVector(kVHF, stats_size + palette_space + extra_space);
  float* RESTRICT stats = result->data();

  size_t pos = 0;
  const auto maybe_add_leaf = [num_non_leaf, stats, &pos](Fragment* leaf) {
    if (leaf->ordinal < num_non_leaf) return;
    const auto sum_rgb1 = load(kVHF, leaf->stats.values);
    const auto pixel_count = broadcast<3>(kVHF, sum_rgb1);
    const auto rgb1 = div(kVHF, sum_rgb1, pixel_count);
    const auto rgbc = blend<0x8>(kVHF, rgb1, pixel_count);
    store(rgbc, kVHF, stats + 4 * pos++);
  };

  for (size_t i = 0; i < num_non_leaf; ++i) {
    Fragment* node = partition[i];
    maybe_add_leaf(node->left_child.get());
    maybe_add_leaf(node->right_child.get());
  }

  result->len = n;
  if (m > 0) makePalette(result->data(), n, m);

  return result;
}

SIMD NOINLINE static float simulateEncode(int32_t target_size,
                                          std::vector<Fragment*>& partition,
                                          const CodecParams& cp) {
  size_t num_non_leaf = subpartition(target_size, partition, cp);
  if (num_non_leaf <= 1) {
    // Let's deal with flat image separately.
    return 1e35f;
  }
  const size_t m = cp.palette_size;
  Owned<Vector<float>> patches_and_palette =
      gatherPatchesAndBuildPalette(partition, num_non_leaf, m);
  size_t n = patches_and_palette->len;
  float* RESTRICT stats = patches_and_palette->data();
  float* RESTRICT colors = stats + 4 * n;
  const auto k0 = zero(kVHF);
  const auto k2 = set1(kVHF, 2.0f);

  // pixel_score = (orig - color)^2
  // patch_score = sum(pixel_score)
  //             = sum(orig^2) + sum(color^2) - 2 * sum(orig * color)
  //             = sum(orig^2) + area * color ^ 2 - 2 * color * sum(orig)
  //             = sum(orig^2) + area * color * (color - 2 * avg(orig))
  // score = sum(patch_score)
  // sum(sum(orig^2)) is a constant => could be omitted from calculations

  auto result_rgbx = k0;
  typedef std::function<__m128(__m128)> ColorTransformer;
  auto accumulate_score = [&result_rgbx, stats, n,
                           &k2](const ColorTransformer& get_color) SIMD {
    for (size_t i = 0; i < n; ++i) {
      const auto rgbc = load(kVHF, stats + 4 * i);
      const auto pixel_count = broadcast<3>(kVHF, rgbc);
      const auto color = round(kVHF, get_color(rgbc));
      const auto t0 = mul(kVHF, rgbc, k2);
      const auto t1 = mul(kVHF, pixel_count, color);
      const auto t2 = sub(kVHF, color, t0);
      const auto t3 = mul(kVHF, t1, t2);
      result_rgbx = add(kVHF, result_rgbx, t3);
    }
  };
  if (m == 0) {
    int32_t v_max = cp.color_quant - 1;
    const auto quant = set1(kVHF, v_max / 255.0f);
    const auto dequant = set1(kVHF, 255.0f / v_max);
    auto quantizer = [&quant, &dequant](__m128 rgbc) SIMD {
      const auto v_scaled = mul(kVHF, quant, rgbc);
      const auto v = round(kVHF, v_scaled);
      return mul(kVHF, v, dequant);
    };
    accumulate_score(quantizer);
  } else {
    auto color_matcher = [colors, m, &k0](__m128 rgbc) SIMD {
      size_t index;
      float dummy;
      chooseColor(blend<0x8>(kVHF, rgbc, k0), colors, m, &index, &dummy);
      return load(kVHF, colors + 4 * index);
    };
    accumulate_score(color_matcher);
  }

  const auto result_rgb0 = blend<0x8>(kVHF, result_rgbx, zero(kVHF));
  return reduce(result_rgb0);
}

static std::vector<uint8_t> doEncode(int32_t target_size,
                                     const std::vector<Fragment*>& partition,
                                     const CodecParams& cp) {
  size_t num_non_leaf = subpartition(target_size, partition, cp);
  const size_t m = cp.palette_size;
  Owned<Vector<float>> patches_and_palette =
      gatherPatchesAndBuildPalette(partition, num_non_leaf, m);
  size_t n = patches_and_palette->len;
  const float* RESTRICT palette = patches_and_palette->data() + 4 * n;

  std::vector<Fragment*> queue;
  queue.reserve(n);
  queue.push_back(partition[0]);

  RangeEncoder dst;
  cp.write(&dst);

  for (size_t j = 0; j < m; ++j) {
    for (size_t c = 0; c < 3; ++c) {
      int32_t clr = static_cast<int32_t>(palette[4 * j + c]);
      RangeEncoder::writeNumber(&dst, 256, clr);
    }
  }

  size_t encoded = 0;
  while (encoded < queue.size()) {
    Fragment* node = queue[encoded++];
    bool is_leaf = (node->ordinal >= num_non_leaf);
    node->encode(&dst, cp, is_leaf, palette, &queue);
  }

  return dst.finish();
}

class SimulationTask {
 public:
  const int32_t target_size;
  const UberCache* uber;
  CodecParams cp;
  float best_sqe = 1e35f;
  int32_t best_color_code = -1;

  SimulationTask(int32_t target_size, const UberCache* uber)
      : target_size(target_size), uber(uber), cp(uber->width, uber->height) {}

  void run() {
    Cache cache(*uber);
    Fragment root = makeRoot(uber->width, uber->height);
    std::vector<Fragment*> partition =
        buildPartition(&root, target_size, cp, &cache);
    for (int32_t color_code = 0; color_code < CodecParams::kMaxColorCode;
         ++color_code) {
      // if (color_code != 1) continue;
      cp.setColorCode(color_code);
      float sqe = simulateEncode(target_size, partition, cp);
      if (sqe < best_sqe) {
        best_sqe = sqe;
        best_color_code = color_code;
      }
    }
    cp.setColorCode(best_color_code);
  }
};

class TaskExecutor {
 public:
  TaskExecutor(size_t max_tasks) { tasks.reserve(max_tasks); }

  void run() {
    while (true) {
      size_t my_task = next_task++;
      if (my_task >= tasks.size()) return;
      tasks[my_task].run();
    }
  }

  std::atomic<size_t> next_task{0};
  std::vector<SimulationTask> tasks;
};

}  // namespace

std::vector<uint8_t> Encoder::encode(const Image& src, int32_t target_size) {
  int32_t width = src.width;
  int32_t height = src.height;
  if (width < 8 || height < 8) {
    fprintf(stderr, "image is too small (%d x %d)\n", width, height);
    return {};
  }
  UberCache uber(src);
  size_t max_tasks =
      CodecParams::kMaxLineLimit * CodecParams::kMaxPartitionCode;
  TaskExecutor executor(max_tasks);
  std::vector<SimulationTask>* tasks = &executor.tasks;
  for (int32_t line_limit = 0; line_limit < CodecParams::kMaxLineLimit;
       ++line_limit) {
    for (int32_t partition_code = 0;
         partition_code < CodecParams::kMaxPartitionCode; ++partition_code) {
      // if (line_limit != 0) continue;
      // if (partition_code != 0) continue;
      tasks->emplace_back(target_size, &uber);
      SimulationTask& task = tasks->back();
      task.cp.setPartitionCode(partition_code);
      task.cp.line_limit = line_limit + 1;
    }
  }

  std::vector<std::future<void>> futures;
  size_t num_cores = 12;  // TODO: get from sys.
  futures.reserve(num_cores);
  for (size_t i = 0; i < num_cores; ++i) {
    futures.push_back(
        std::async(std::launch::async, &TaskExecutor::run, &executor));
  }
  for (size_t i = 0; i < num_cores; ++i) futures[i].get();

  size_t best_task_index = 0;
  float best_sqe = 1e35f;
  for (size_t task_index = 0; task_index < tasks->size(); ++task_index) {
    const SimulationTask& task = tasks->at(task_index);
    if (task.best_sqe < best_sqe) {
      best_task_index = task_index;
      best_sqe = task.best_sqe;
    }
  }
  best_sqe += uber.rgb2[0] + uber.rgb2[1] + uber.rgb2[2];
  SimulationTask& best_task = tasks->at(best_task_index);
  best_task.cp.setColorCode(best_task.best_color_code);
  Fragment root = makeRoot(width, height);
  Cache cache(uber);
  std::vector<Fragment*> partition =
      buildPartition(&root, target_size, best_task.cp, &cache);
  std::vector<uint8_t> result = doEncode(target_size, partition, best_task.cp);

  std::string best_cp = best_task.cp.toString();
  fprintf(stdout, "size: %zu, cp: [%s], error: %.3f\n", result.size(),
          best_cp.c_str(), std::sqrt(best_sqe / (width * height)));
  return result;
}

}  // namespace twim
