//#include "encoder.h"

#include <cmath>
#include <future>
#include <hwy/highway.h>
#include <list>
#include <queue>
#include <vector>

#include "encoder.h"
#include "codec_params.h"
#include "distance_range.h"
#include "image.h"
#include "platform.h"
#include "region.h"
#include "xrange_encoder.h"

namespace twim {

using ::twim::Encoder::Result;
using ::twim::Encoder::Variant;

// TODO(eustas): move to common header
// TODO(eustas): adjust to SIMD implementation
constexpr size_t kDefaultAlign = 32;

/*
 * https://xkcd.com/221/
 *
 * Chosen by fair dice roll. Guaranteed to be random.
 */
constexpr float kRandom[64] = {
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

struct Stats {
  /* sum_r, sum_g, sum_b, pixel_count */
  ALIGNED_16 float values[4];
};

INLINE float rgb(const Stats& stats, size_t c) { return stats.values[c]; }

INLINE float pixelCount(const Stats& stats) { return stats.values[3]; }

class UberCache {
 public:
  const uint32_t width;
  const uint32_t height;
  const uint32_t stride;
  /* Cumulative sums. Extra column with total sum. */
  Owned<Vector<float>> sum;

  float rgb2[3] = {0.0f};

  explicit UberCache(const Image& src)
      : width(src.width),
        height(src.height),
        // 4 == [r, g, b, count].length
        stride(vecSize<float, kDefaultAlign>(4 * (src.width + 1))),
        sum(allocVector<float, kDefaultAlign>(stride * src.height)) {
    float* RESTRICT sum = this->sum->data();
    for (size_t y = 0; y < src.height; ++y) {
      float row_rgb2[3] = {0.0f};
      size_t src_row_offset = y * src.width;
      const uint8_t* RESTRICT r_row = src.r.data() + src_row_offset;
      const uint8_t* RESTRICT g_row = src.g.data() + src_row_offset;
      const uint8_t* RESTRICT b_row = src.b.data() + src_row_offset;
      size_t dstRowOffset = y * this->stride;
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
      for (size_t c = 0; c < 3; ++c) this->rgb2[c] += row_rgb2[c];
    }
  }
};

class Cache {
 public:
  const UberCache* uber;
  Stats plus;
  Stats minus;
  Stats stats[CodecParams::kMaxLineLimit + 3];

  uint32_t count;
  Owned<Vector<int32_t>> row_offset;
  Owned<Vector<float>> y;
  Owned<Vector<int32_t>> x0;
  Owned<Vector<int32_t>> x1;
  Owned<Vector<int32_t>> x;

  explicit Cache(const UberCache& uber)
      : uber(&uber),
        row_offset(allocVector<int32_t, kDefaultAlign>(uber.height)),
        y(allocVector<float, kDefaultAlign>(uber.height)),
        x0(allocVector<int32_t, kDefaultAlign>(uber.height)),
        x1(allocVector<int32_t, kDefaultAlign>(uber.height)),
        x(allocVector<int32_t, kDefaultAlign>(uber.height)) {}
};

class Fragment {
 public:
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

  Stats stats;

  // Subdivision.
  uint32_t ordinal = 0x7FFFFFFF;
  int32_t level;
  uint32_t best_angle_code;
  uint32_t best_line;
  float best_score;
  uint32_t best_num_lines;
  float best_cost;

  Fragment(Fragment&&) = default;
  Fragment& operator=(Fragment&&) = default;
  Fragment(const Fragment&) = delete;
  Fragment& operator=(const Fragment&) = delete;

  explicit Fragment(Owned<Vector<int32_t>>&& region)
    : region(std::move(region)) {}

  template <typename EntropyEncoder>
  NOINLINE void encode(EntropyEncoder* dst, const CodecParams& cp, bool is_leaf,
                       const float* RESTRICT palette,
                       std::vector<Fragment*>* children);
};

class Partition {
 public:
  Partition(const UberCache& uber, const CodecParams& cp, size_t target_size);

  const std::vector<Fragment*>* getPartition() const;

  // CodecParams should be the same as passed to constructor; only color code
  // is allowed to be different.
  uint32_t subpartition(const CodecParams& cp, uint32_t target_size) const;

 private:
  Cache cache;
  Fragment root;
  std::vector<Fragment*> partition;
};

float bitCost(int32_t range) {
  static float kInvLog2 = 1.0f / std::log(2.0f);
  return static_cast<float>(std::log(range) * kInvLog2);
}

struct FragmentComparator {
  bool operator()(const Fragment* left, const Fragment* right) const {
    return left->best_score < right->best_score;
  }
};

Fragment makeRoot(uint32_t width, uint32_t height) {
  uint32_t step = vecSize<int32_t, kDefaultAlign>(height);
  Owned<Vector<int32_t>> root = allocVector<int32_t, kDefaultAlign>(3 * step);
  int32_t* RESTRICT data = root->data();
  for (uint32_t y = 0; y < height; ++y) {
    data[y] = y;
    data[step + y] = 0;
    data[2 * step + y] = width;
  }
  root->len = height;
  return Fragment(std::move(root));
}

#include <hwy/before_namespace-inl.h>
#include <hwy/begin_target-inl.h>

constexpr HWY_CAPPED(float, 4) kVF;
constexpr HWY_CAPPED(int32_t, 4) kVI32;
using F32x4 = Vec<HWY_CAPPED(float, 4)>;

void checkSane() {
  if ((Lanes(kVF) != 4) || (Lanes(kVI32) != 4)) __builtin_trap();
}

INLINE void reset(Stats* stats) { Store(Zero(kVF), kVF, stats->values); }

INLINE void diff(Stats* diff, const Stats& plus, const Stats& minus) {
  const auto p = Load(kVF, plus.values);
  const auto m = Load(kVF, minus.values);
  Store(p - m, kVF, diff->values);
}

INLINE void copy(Stats* to, const Stats& from) {
  Store(Load(kVF, from.values), kVF, to->values);
}

void INLINE updateGeHorizontal(Cache* cache, int32_t d) {
  int32_t ny = SinCos::kCos[0];
  float dny = d / (float)ny;  // TODO: precalculate
  size_t count = cache->count;
  int32_t* RESTRICT row_offset = cache->row_offset->data();
  float* RESTRICT region_y = cache->y->data();
  int32_t* RESTRICT region_x0 = cache->x0->data();
  int32_t* RESTRICT region_x1 = cache->x1->data();
  int32_t* RESTRICT region_x = cache->x->data();

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

INLINE void updateGeGeneric(Cache* cache, int32_t angle, int32_t d) {
  float m_ny_nx = SinCos::kMinusCot[angle];
  float d_nx = static_cast<float>(d * SinCos::kInvSin[angle] + 0.5);
  int32_t* RESTRICT row_offset = cache->row_offset->data();
  float* RESTRICT region_y = cache->y->data();
  int32_t* RESTRICT region_x0 = cache->x0->data();
  int32_t* RESTRICT region_x1 = cache->x1->data();
  int32_t* RESTRICT region_x = cache->x->data();

  const auto& vf = kVF;
  const auto& vi32 = kVI32;
  const auto k4 = Set(vi32, 4);
  const auto d_nx_ = Set(vf, d_nx);
  const auto m_ny_nx_ = Set(vf, m_ny_nx);
  const size_t count = cache->count;
  for (size_t i = 0; i < count; i += Lanes(vf)) {
    const auto y = Load(vf, region_y + i);
    const auto offset = Load(vi32, row_offset + i);
    const auto xf = MulAdd(y, m_ny_nx_, d_nx_);
    const auto xi = ConvertTo(vi32, xf);
    const auto x0 = Load(vi32, region_x0 + i);
    const auto x1 = Load(vi32, region_x1 + i);
    const auto x = Min(x1, Max(xi, x0));
    const auto x_off = k4 * x + offset;
    Store(x_off, vi32, region_x + i);
  }
}

NOINLINE void updateGe(Cache* cache, int angle, int d) {
  if (angle == 0) {
    updateGeHorizontal(cache, d);
  } else {
    updateGeGeneric(cache, angle, d);
  }
}

NOINLINE float score(const Stats& whole, const Stats& left,
                     const Stats& right) {
  if ((pixelCount(left) <= 0.0f) || (pixelCount(right) <= 0.0f)) return 0.0f;

  const auto k1 = Set(kVF, 1.0f);
  HWY_ALIGN float pc_array[4] = {pixelCount(whole), pixelCount(left),
                                pixelCount(right), 1.0f};
  const auto pc = Load(kVF, pc_array);
  const auto inv_pc = k1 / pc;

  const auto k2 = Set(kVF, 2.0f);
  const auto whole_values = Load(kVF, whole.values);
  const auto whole_average = whole_values * Broadcast<0>(inv_pc);

  const auto left_values = Load(kVF, left.values);
  const auto left_pixel_count = Set(kVF, pixelCount(left));
  const auto left_average = left_values *  Broadcast<1>(inv_pc);
  const auto left_plus = whole_average + left_average;
  const auto left_minus = whole_average - left_average;
  const auto left_a = left_pixel_count * left_plus;
  const auto left_b = k2 * left_values;
  const auto left_sum = left_minus * (left_a - left_b);

  const auto right_values = Load(kVF, right.values);
  const auto right_pixel_count = Set(kVF, pixelCount(right));
  const auto right_average = right_values *  Broadcast<2>(inv_pc);
  const auto right_plus = whole_average + right_average;
  const auto right_minus = whole_average - right_average;
  const auto right_a = right_pixel_count * right_plus;
  const auto right_b = k2 * right_values;
  const auto right_sum = right_minus * (right_a - right_b);

  return GetLane(SumOfLanes(left_sum + right_sum));
}

INLINE void chooseColor(const F32x4 rgb0, const float* RESTRICT palette,
                        uint32_t palette_size, uint32_t* RESTRICT index,
                        float* RESTRICT distance2) {
  const size_t m = palette_size;
  uint32_t best_j = 0;
  float best_d2 = 1e35f;
  for (uint32_t j = 0; j < m; ++j) {
    const auto center = Load(kVF, palette + 4 * j);
    const auto d = rgb0 - center;
    float d2 = GetLane(SumOfLanes(d * d));
    if (d2 < best_d2) {
      best_j = j;
      best_d2 = d2;
    }
  }
  *index = best_j;
  *distance2 = best_d2;
}

NOINLINE uint32_t chooseColor(float r, float g, float b,
                              const float* RESTRICT palette,
                              uint32_t palette_size) {
  uint32_t index;
  float dummy;
  HWY_ALIGN float rgb0[4];
  rgb0[0] = r;
  rgb0[1] = g;
  rgb0[2] = b;
  rgb0[3] = 0.0f;
  chooseColor(Load(kVF, rgb0), palette, palette_size, &index, &dummy);
  return index;
}

INLINE void makePalette(const float* stats, float* RESTRICT storage,
                        uint32_t num_patches, uint32_t palette_size) {
  const uint32_t n = num_patches;
  const uint32_t m = palette_size;
  float* RESTRICT centers = storage;
  float* RESTRICT centers_acc = centers + 4 * m;
  float* RESTRICT weights = centers_acc + 4 * m;
  const auto k0 = Zero(kVF);
  HWY_ALIGN const int32_t kRgbMask[4] = {-1, -1, -1, 0};
  const auto rgb_mask = BitCast(kVF, Load(kVI32, kRgbMask));
  HWY_ALIGN const float kPcOne[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  const auto pc_one = Load(kVF, kPcOne);

  {
    // Choose one center uniformly at random from among the data points.
    float total = 0.0f;
    uint32_t i;
    for (i = 0; i < n; ++i) total += stats[4 * i + 3];
    float target = total * kRandom[0];
    float partial = 0.0f;
    for (i = 0; i < n; ++i) {
      partial += stats[4 * i + 3];
      if (partial >= target) break;
    }
    i = std::max(i, n - 1);
    const auto rgbc = Load(kVF, stats + 4 * i);
    const auto rgb0 = And(rgb_mask, rgbc);
    Store(rgb0, kVF, centers);
  }

  for (uint32_t j = 1; j < m; ++j) {
    // D(x) is the distance to the nearest center.
    // Choose next with probability proportional to D(x)^2.
    uint32_t i;
    float total = 0.0f;
    for (i = 0; i < n; ++i) {
      const auto rgbc = Load(kVF, stats + 4 * i);
      const auto rgb0 = And(rgb_mask, rgbc);
      float best_distance = 1e35f;
      for (size_t k = 0; k < j; ++k) {
        const auto center = Load(kVF, centers + 4 * k);
        const auto d = rgb0 - center;
        float distance = GetLane(SumOfLanes(d * d));
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
    const auto rgbc = Load(kVF, stats + 4 * i);
    const auto rgb0 = And(rgb_mask, rgbc);
    Store(rgb0, kVF, centers + 4 * j);
  }

  float last_score = 1e35f;
  while (true) {
    for (size_t j = 0; j < m; ++j) Store(k0, kVF, centers_acc + 4 * j);
    float score = 0.0f;
    for (size_t i = 0; i < n; ++i) {
      const auto rgbc = Load(kVF, stats + 4 * i);
      const auto rgb0 = And(rgb_mask, rgbc);
      const auto rgb1 = rgb0 + pc_one;
      uint32_t index;
      float d2;
      chooseColor(rgb0, centers, m, &index, &d2);
      const float pixel_count_value = stats[4 * i + 3];
      const auto pixel_count = Set(kVF, pixel_count_value);
      score += d2 * pixel_count_value;
      const auto weighted_rgbc = rgb1 * pixel_count;
      auto center_acc = Load(kVF, centers_acc + 4 * index);
      center_acc = center_acc + weighted_rgbc;
      Store(center_acc, kVF, centers_acc + 4 * index);
    }
    for (size_t j = 0; j < m; ++j) {
      const float pixel_count_value = centers_acc[4 * j + 3];
      const auto pixel_count = Set(kVF, pixel_count_value);
      if (pixel_count_value < 0.5f) {
        // Ooops, an orphaned center... Let it be.
      } else {
        const auto weighted_rgbc = Load(kVF, centers_acc + 4 * j);
        const auto rgb1 = weighted_rgbc / pixel_count;
        const auto rgb0 = And(rgb_mask, rgb1);
        Store(rgb0, kVF, centers + 4 * j);
      }
    }
    if (last_score - score < 1.0f) break;
    last_score = score;
  }
}

NOINLINE Owned<Vector<float>> buildPalette(const Owned<Vector<float>>& patches,
                                           uint32_t palette_size) {
  uint32_t n = patches->len;
  uint32_t m = palette_size;
  uint32_t palette_space = 4 * m;
  uint32_t extra_space = 4 * m + ((m > 0) ? n : 1);
  Owned<Vector<float>> result = allocVector<float, kDefaultAlign>(palette_space + extra_space);
  const float* RESTRICT stats = patches->data();

  result->len = m;
  if (m > 0) {
    makePalette(stats, result->data(), n, m);

    // Round the pallete values.
    float* RESTRICT colors = result->data();
    const auto k05 = Set(kVF, 0.5f);
    for (size_t i = 0; i < m; ++i) {
      auto rgbx = Load(kVF, colors + 4 * i);
      rgbx = rgbx + k05;
      rgbx = ConvertTo(kVF, ConvertTo(kVI32, rgbx));
      Store(rgbx, kVF, colors + 4 * i);
    }
  }

  return result;
}

NOINLINE Owned<Vector<float>> gatherPatches(
    const std::vector<Fragment*>* partition, uint32_t num_non_leaf) {
  /* In a binary tree the number of leaves is number of nodes plus one. */
  uint32_t n = num_non_leaf + 1;
  uint32_t stats_size = 4 * vecSize<float, kDefaultAlign>(n);
  Owned<Vector<float>> result = allocVector<float, kDefaultAlign>(stats_size);
  float* RESTRICT stats = result->data();

  HWY_ALIGN const int32_t kRgbMask[4] = {-1, -1, -1, 0};
  const auto rgb_mask = BitCast(kVF, Load(kVI32, kRgbMask));
  HWY_ALIGN const int32_t kPcMask[4] = {0, 0, 0, -1};
  const auto pc_mask = BitCast(kVF, Load(kVI32, kPcMask));

  size_t pos = 0;
  const auto maybe_add_leaf = [&](Fragment* leaf) {
    if (leaf->ordinal < num_non_leaf) return;
    const auto sum_rgb1 = Load(kVF, leaf->stats.values);
    const auto pixel_count = Broadcast<3>(sum_rgb1);
    const auto rgb1 = sum_rgb1 / pixel_count;
    const auto rgb0 = And(rgb_mask, rgb1);
    const auto rgbc = rgb0 + And(pc_mask, pixel_count);
    Store(rgbc, kVF, stats + 4 * pos++);
  };

  for (size_t i = 0; i < num_non_leaf; ++i) {
    Fragment* node = partition->at(i);
    maybe_add_leaf(node->left_child.get());
    maybe_add_leaf(node->right_child.get());
  }

  result->len = n;
  return result;
}

NOINLINE float simulateEncode(const Partition& partition_holder,
                              uint32_t target_size, const CodecParams& cp) {
  uint32_t num_non_leaf = partition_holder.subpartition(cp, target_size);
  if (num_non_leaf <= 1) {
    // Let's deal with flat image separately.
    return 1e35f;
  }
  Owned<Vector<float>> patches =
      gatherPatches(partition_holder.getPartition(), num_non_leaf);
  float* RESTRICT stats = patches->data();

  size_t n = patches->len;
  const uint32_t m = cp.palette_size;
  Owned<Vector<float>> palette = buildPalette(patches, m);
  float* RESTRICT colors = palette->data();
  const auto k0 = Zero(kVF);
  const auto k05 = Set(kVF, 0.5f);
  HWY_ALIGN const int32_t kRgbMask[4] = {-1, -1, -1, 0};
  const auto rgb_mask = BitCast(kVF, Load(kVI32, kRgbMask));

  // pixel_score = (orig - color)^2
  // patch_score = sum(pixel_score)
  //             = sum(orig^2) + sum(color^2) - 2 * sum(orig * color)
  //             = sum(orig^2) + area * color ^ 2 - 2 * color * sum(orig)
  //             = sum(orig^2) + area * color * (color - 2 * avg(orig))
  // score = sum(patch_score)
  // sum(sum(orig^2)) is a constant => could be omitted from calculations

  auto result_rgbx = k0;
  typedef std::function<F32x4(F32x4)> ColorTransformer;
  auto accumulate_score = [&](const ColorTransformer& get_color) {
    for (size_t i = 0; i < n; ++i) {
      const auto rgbc = Load(kVF, stats + 4 * i);
      const auto pixel_count = Broadcast<3>(rgbc);
      const auto color = get_color(rgbc);
      const auto t0 = rgbc + rgbc;
      const auto t1 = pixel_count * color;
      const auto t2 = color - t0;
      const auto t3 = t1 * t2;
      result_rgbx = result_rgbx + t3;
    }
  };
  if (m == 0) {
    int32_t v_max = cp.color_quant - 1;
    const auto quant = Set(kVF, v_max / 255.0f);
    const auto dequant = Set(kVF, 255.0f / v_max);
    auto quantizer = [&](F32x4 rgbc) {
      const auto v_scaled = MulAdd(quant, rgbc, k05);
      const auto v = ConvertTo(kVF, ConvertTo(kVI32, v_scaled));
      const auto color = v * dequant;
      return ConvertTo(kVF, ConvertTo(kVI32, color));
    };
    accumulate_score(quantizer);
  } else {
    auto color_matcher = [&](F32x4 rgbc) {
      uint32_t index;
      float dummy;
      chooseColor(And(rgbc, rgb_mask), colors, m, &index, &dummy);
      return Load(kVF, colors + 4 * index);
    };
    accumulate_score(color_matcher);
  }

  const auto result_rgb0 = And(result_rgbx, rgb_mask);
  return GetLane(SumOfLanes(result_rgb0));
}

void sumCache(const Cache* c, const int32_t* RESTRICT region_x, Stats* dst) {
  size_t count = c->count;
  const int32_t* RESTRICT row_offset = c->row_offset->data();
  const float* RESTRICT sum = c->uber->sum->data();

  auto tmp = Zero(kVF);
  for (size_t i = 0; i < count; i += Lanes(kVF)) {
    // TODO(eustas): why 2 loops?
    for (size_t j = 0; j < Lanes(kVF); ++j) {
      int32_t offset = row_offset[i + j] + 4 * region_x[i + j];
      tmp = tmp + Load(kVF, sum + offset);
    }
  }
  Store(tmp, kVF, dst->values);
}

void sumCacheAbs(const Cache* c, const int32_t* RESTRICT region_x, Stats* dst) {
  size_t count = c->count;
  const float* RESTRICT sum = c->uber->sum->data();

  auto tmp = Zero(kVF);
  for (size_t i = 0; i < count; i += Lanes(kVF)) {
    // TODO(eustas): why 2 loops?
    for (size_t j = 0; j < Lanes(kVF); ++j) {
      tmp = tmp + Load(kVF, sum + region_x[i + j]);
    }
  }
  Store(tmp, kVF, dst->values);
}

void INLINE prepareCache(Cache* c, Vector<int32_t>* region) {
  uint32_t count = region->len;
  uint32_t step = region->capacity / 3;
  uint32_t sum_stride = c->uber->stride;
  int32_t* RESTRICT data = region->data();
  float* RESTRICT y = c->y->data();
  int32_t* RESTRICT x0 = c->x0->data();
  int32_t* RESTRICT x1 = c->x1->data();
  int32_t* RESTRICT row_offset = c->row_offset->data();
  for (size_t i = 0; i < count; ++i) {
    int32_t row = data[i];
    y[i] = row;
    x0[i] = data[step + i];
    x1[i] = data[2 * step + i];
    row_offset[i] = row * sum_stride;
  }
  // TODO(eustas): 3 == kStride - 1
  while ((count & 3) != 0) {
    y[count] = 0;
    x0[count] = 0;
    x1[count] = 0;
    row_offset[count] = 0;
    count++;
  }
  c->count = count;
}

void NOINLINE findBestSubdivision(Fragment* f, Cache* cache, CodecParams cp) {
  Vector<int32_t>& region = *f->region;
  Stats& stats = f->stats;
  Stats* cache_stats = cache->stats;
  uint32_t level = cp.getLevel(region);
  // TODO(eustas): assert level is not kInvalid.
  uint32_t angle_max = 1u << cp.angle_bits[level];
  uint32_t angle_mult = (SinCos::kMaxAngle / angle_max);
  uint32_t best_angle_code = 0;
  uint32_t best_line = 0;
  float best_score = -1.0f;
  prepareCache(cache, &region);
  sumCache(cache, cache->x1->data(), &cache->plus);
  sumCache(cache, cache->x0->data(), &cache->minus);
  diff(&stats, cache->plus, cache->minus);

  // Find subdivision
  for (uint32_t angle_code = 0; angle_code < angle_max; ++angle_code) {
    int32_t angle = angle_code * angle_mult;
    DistanceRange distance_range(region, angle, cp);
    uint32_t num_lines = distance_range.num_lines;
    reset(&cache_stats[0]);
    for (uint32_t line = 0; line < num_lines; ++line) {
      updateGe(cache, angle, distance_range.distance(line));
      sumCacheAbs(cache, cache->x->data(), &cache->minus);
      diff(&cache_stats[line + 1], cache->plus, cache->minus);
    }
    copy(&cache_stats[num_lines + 1], stats);

    for (uint32_t line = 0; line < num_lines; ++line) {
      float full_score = 0.0f;
      /*
      constexpr const float kLocalWeight = 0.0f;
      if (kLocalWeight != 0.0f) {
        Stats band;
        diff(&band, cache_stats[line + 2], cache_stats[line]);
        Stats left;
        diff(&left, band, cache_stats[line + 1]);
        Stats right;
        diff(&right, band, left);
        full_score += kLocalWeight * (score(band, left, right));
      }
      */

      {
        Stats right;
        diff(&right, stats, cache_stats[line + 1]);
        full_score += score(stats, cache_stats[line + 1], right);
      }

      if (full_score > best_score) {
        best_angle_code = angle_code;
        best_line = line;
        best_score = full_score;
      }
    }
  }

  f->level = level;
  f->best_score = best_score;

  if (best_score < 0.0f) {
    f->best_cost = -1.0f;
  } else {
    DistanceRange distance_range(region, best_angle_code * angle_mult, cp);
    uint32_t child_step = vecSize<int32_t, kDefaultAlign>(region.len);
    f->left_child.reset(new Fragment(allocVector<int32_t, kDefaultAlign>(3 * child_step)));
    f->right_child.reset(new Fragment(allocVector<int32_t, kDefaultAlign>(3 * child_step)));
    Region::splitLine(region, best_angle_code * angle_mult,
                      distance_range.distance(best_line),
                      f->left_child->region.get(),
                      f->right_child->region.get());

    f->best_angle_code = best_angle_code;
    f->best_num_lines = distance_range.num_lines;
    f->best_line = best_line;
    f->best_cost =
        bitCost(NodeType::COUNT * angle_max * distance_range.num_lines);
  }
}

#include <hwy/end_target-inl.h>
#include <hwy/after_namespace-inl.h>

class SimulationTask {
 public:
  const uint32_t targetSize;
  Variant variant;
  const UberCache* uber;
  CodecParams cp;
  float bestSqe = 1e35f;
  uint32_t bestColorCode = (uint32_t)-1;
  std::unique_ptr<Partition> partitionHolder;

  SimulationTask(uint32_t targetSize, Variant variant, const UberCache& uber)
      : targetSize(targetSize),
        variant(variant),
        uber(&uber),
        cp(uber.width, uber.height) {}

  void run() {
    cp.setPartitionCode(variant.partitionCode);
    cp.line_limit = variant.lineLimit + 1;
    uint64_t colorOptions = variant.colorOptions;
    // TODO: color-options based taxes
    partitionHolder.reset(new Partition(*uber, cp, targetSize));
    for (uint32_t colorCode = 0; colorCode < CodecParams::kMaxColorCode;
         ++colorCode) {
      if (!(colorOptions & (1 << colorCode))) continue;
      cp.setColorCode(colorCode);
      float sqe = HWY_STATIC_DISPATCH(simulateEncode)(*partitionHolder, targetSize, cp);
      if (sqe < bestSqe) {
        bestSqe = sqe;
        bestColorCode = colorCode;
      }
    }
  }
};

class TaskExecutor {
 public:
  explicit TaskExecutor(size_t max_tasks) { tasks.reserve(max_tasks); }

  void run() {
    float bestSqe = 1e35f;
    size_t lastBestTask = (size_t)-1;

    while (true) {
      size_t myTask = nextTask++;
      if (myTask >= tasks.size()) return;
      SimulationTask& task = tasks[myTask];
      task.run();
      if (task.bestSqe < bestSqe) {
        bestSqe = task.bestSqe;
        if (lastBestTask < myTask) {
          tasks[lastBestTask].partitionHolder.reset();
        }
        lastBestTask = myTask;
      } else {
        task.partitionHolder.reset();
      }
    }
  }

  std::atomic<size_t> nextTask{0};
  std::vector<SimulationTask> tasks;
};

template <typename EntropyEncoder>
NOINLINE void Fragment::encode(EntropyEncoder* dst, const CodecParams& cp,
                               bool is_leaf, const float* RESTRICT palette,
                               std::vector<Fragment*>* children) {
  if (is_leaf) {
    EntropyEncoder::writeNumber(dst, NodeType::COUNT, NodeType::FILL);
    if (cp.palette_size == 0) {
      float quant = static_cast<float>(cp.color_quant - 1) / 255.0f;
      for (size_t c = 0; c < 3; ++c) {
        uint32_t v = static_cast<uint32_t>(
            roundf(quant * rgb(stats, c) / pixelCount(stats)));
        EntropyEncoder::writeNumber(dst, cp.color_quant, v);
      }
    } else {
      float r = rgb(stats, 0) / pixelCount(stats);
      float g = rgb(stats, 1) / pixelCount(stats);
      float b = rgb(stats, 2) / pixelCount(stats);
      uint32_t index = HWY_STATIC_DISPATCH(chooseColor)(r, g, b, palette, cp.palette_size);
      EntropyEncoder::writeNumber(dst, cp.palette_size, index);
    }
    return;
  }
  EntropyEncoder::writeNumber(dst, NodeType::COUNT, NodeType::HALF_PLANE);
  uint32_t maxAngle = 1u << cp.angle_bits[level];
  EntropyEncoder::writeNumber(dst, maxAngle, best_angle_code);
  EntropyEncoder::writeNumber(dst, best_num_lines, best_line);
  children->push_back(left_child.get());
  children->push_back(right_child.get());
}

template<typename EntropyEncoder>
std::vector<uint8_t> doEncode(uint32_t num_non_leaf,
                              const std::vector<Fragment*>* partition,
                              const CodecParams& cp,
                              const float* RESTRICT palette) {
  const uint32_t m = cp.palette_size;
  uint32_t n = num_non_leaf + 1;

  std::vector<Fragment*> queue;
  queue.reserve(n);
  queue.push_back(partition->at(0));

  EntropyEncoder dst;
  cp.write(&dst);

  for (uint32_t j = 0; j < m; ++j) {
    for (size_t c = 0; c < 3; ++c) {
      uint32_t clr = static_cast<uint32_t>(palette[4 * j + c]);
      EntropyEncoder::writeNumber(&dst, 256, clr);
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

/**
 * Builds the space partition.
 *
 * Minimal color data cost is used.
 * Partition could be used to try multiple color quantization to see, which one
 * gives the best result.
 */
NOINLINE std::vector<Fragment*> buildPartition(Fragment* root,
                                               size_t size_limit,
                                               const CodecParams& cp,
                                               Cache* cache) {
  float tax = bitCost(NodeType::COUNT);
  float budget = size_limit * 8.0f - tax -
                 cp.getTax();  // Minus flat image cost.

  std::vector<Fragment*> result;
  std::priority_queue<Fragment*, std::vector<Fragment*>, FragmentComparator>
      queue;
  HWY_STATIC_DISPATCH(findBestSubdivision)(root, cache, cp);
  queue.push(root);
  while (!queue.empty()) {
    Fragment* candidate = queue.top();
    queue.pop();
    if (candidate->best_score < 0.0 || candidate->best_cost < 0.0) break;
    if (tax + candidate->best_cost <= budget) {
      budget -= tax + candidate->best_cost;
      candidate->ordinal = static_cast<uint32_t>(result.size());
      result.push_back(candidate);
      HWY_STATIC_DISPATCH(findBestSubdivision)(candidate->left_child.get(), cache, cp);
      queue.push(candidate->left_child.get());
      HWY_STATIC_DISPATCH(findBestSubdivision)(candidate->right_child.get(), cache, cp);
      queue.push(candidate->right_child.get());
    }
  }
  return result;
}

Partition::Partition(const UberCache& uber, const CodecParams& cp,
                     size_t target_size)
    : cache(uber),
      root(makeRoot(uber.width, uber.height)),
      partition(buildPartition(&root, target_size, cp, &cache)) {}

const std::vector<Fragment*>* Partition::getPartition() const {
  return &partition;
}

/* Returns the index of the first leaf node in partition. */
uint32_t Partition::subpartition(const CodecParams& cp,
                                 uint32_t target_size) const {
  const std::vector<Fragment*>* partition = getPartition();
  float node_tax = bitCost(NodeType::COUNT);
  float image_tax = cp.getTax();
  if (cp.palette_size == 0) {
    node_tax += 3.0f * bitCost(cp.color_quant);
  } else {
    node_tax += bitCost(cp.palette_size);
    image_tax += cp.palette_size * 3.0f * 8.0f;
  }
  float budget = 8.0f * target_size - 4.0f - image_tax - node_tax;
  uint32_t limit = static_cast<uint32_t>(partition->size());
  uint32_t i;
  for (i = 0; i < limit; ++i) {
    Fragment* node = partition->at(i);
    if (node->best_cost < 0.0f) break;
    float cost = node->best_cost + node_tax;
    if (budget < cost) break;
    budget -= cost;
  }
  return i;
}

namespace Encoder {

template <typename EntropyEncoder>
Result encode(const Image& src, const Params& params) {
  Result result;

  HWY_STATIC_DISPATCH(checkSane)();

  int32_t width = src.width;
  int32_t height = src.height;
  if (width < 8 || height < 8) {
    fprintf(stderr, "image is too small (%d x %d)\n", width, height);
    return {};
  }
  UberCache uber(src);
  const std::vector<Variant>& variants = params.variants;
  TaskExecutor executor(variants.size());
  std::vector<SimulationTask>& tasks = executor.tasks;
  for (auto& variant : variants) {
    tasks.emplace_back(params.targetSize, variant, uber);
  }

  std::vector<std::future<void>> futures;
  futures.reserve(params.numThreads);
  bool singleThreaded = (params.numThreads == 1);
  for (uint32_t i = 0; i < params.numThreads; ++i) {
    futures.push_back(
        std::async(singleThreaded ? std::launch::deferred : std::launch::async,
                   &TaskExecutor::run, &executor));
  }
  for (uint32_t i = 0; i < params.numThreads; ++i) futures[i].get();

  size_t bestTaskIndex = 0;
  float bestSqe = 1e35f;
  for (size_t taskIndex = 0; taskIndex < tasks.size(); ++taskIndex) {
    const SimulationTask& task = tasks[taskIndex];
    if (!task.partitionHolder) continue;
    if (task.bestSqe < bestSqe) {
      bestTaskIndex = taskIndex;
      bestSqe = task.bestSqe;
    }
  }
  bestSqe += uber.rgb2[0] + uber.rgb2[1] + uber.rgb2[2];
  SimulationTask& bestTask = tasks[bestTaskIndex];
  CodecParams& cp = bestTask.cp;
  uint32_t bestColorCode  = bestTask.bestColorCode;
  cp.setColorCode(bestColorCode);
  const Partition& partitionHolder = *bestTask.partitionHolder;

  // Encoder workflow >>
  // Partition partitionHolder(uber, cp, params.targetSize);
  uint32_t numNonLeaf = partitionHolder.subpartition(cp, params.targetSize);
  const std::vector<Fragment*>* partition = partitionHolder.getPartition();
  Owned<Vector<float>> patches = HWY_STATIC_DISPATCH(gatherPatches)(partition, numNonLeaf);
  Owned<Vector<float>> palette = HWY_STATIC_DISPATCH(buildPalette)(patches, cp.palette_size);
  float* RESTRICT colors = palette->data();
  result.data = doEncode<EntropyEncoder>(numNonLeaf, partition, cp, colors);
  // << Encoder workflow

  result.variant = bestTask.variant;
  result.variant.colorOptions = (uint64_t)1 << bestColorCode;
  result.mse = bestSqe / static_cast<float>(width * height);
  return result;
}

template Result encode<XRangeEncoder>(const Image& src, const Params& params);

}  // namespace Encoder

}  // namespace twim
