#include "encoder.h"

#include <cmath>
#include <future>
#include <list>
#include <queue>
#include <unordered_set>

#include "codec_params.h"
#include "distance_range.h"
#include "image.h"
#include "platform.h"
#include "region.h"

namespace twim {

namespace {

class Cache;

constexpr auto kVHF = SseVecTag<float>();
static_assert(kVHF.N == 4, "Expected vector size is 4");

constexpr auto kVHI32 = SseVecTag<int32_t>();
static_assert(kVHI32.N == 4, "Expected vector size is 4");

constexpr auto kVF = AvxVecTag<float>();
static_assert(kVF.N == 8, "Expected vector size is 8");

constexpr auto kVI32 = AvxVecTag<int32_t>();
static_assert(kVI32.N == 8, "Expected vector size is 8");

constexpr const size_t kStride = kVHF.N;

class Stats {
 public:
  // r, g, b, pixel_count
  ALIGNED_16 float values[4];

  INLINE float rgb(size_t c) const { return values[c]; }
  INLINE float pixelCount() const { return values[3]; }

  SIMD INLINE void zero() { store(::twim::zero(kVHF), kVHF, values); }

  SIMD INLINE void add(const Stats& a, const Stats& b) {
    const auto av = load(kVHF, a.values);
    const auto bv = load(kVHF, b.values);
    store(::twim::add(kVHF, av, bv), kVHF, values);
  }

  SIMD INLINE void sub(const Stats& plus, const Stats& minus) {
    const auto p = load(kVHF, plus.values);
    const auto m = load(kVHF, minus.values);
    store(::twim::sub(kVHF, p, m), kVHF, values);
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
    const auto xf = ::twim::add(vf, mul(vf, y, mnynx_), dnx_);
    const auto xi = cast(vf, vi32, xf);
    const auto x0 = load(vi32, region_x0 + i);
    const auto x1 = load(vi32, region_x1 + i);
    const auto x = min(vi32, x1, max(vi32, xi, x0));
    const auto x_off = ::twim::add(vi32, mul(vi32, k4, x), offset);
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

static SIMD INLINE float score(const Stats& whole, const Stats& left,
                               const Stats& right) {
  if ((left.pixelCount() <= 0.0f) || (right.pixelCount() <= 0.0f)) return 0.0f;

  const auto k2 = set1(kVHF, 2.0f);
  const auto whole_values = load(kVHF, whole.values);
  const auto whole_pixel_count = set1(kVHF, whole.pixelCount());
  const auto whole_average = div(kVHF, whole_values, whole_pixel_count);

  const auto left_values = load(kVHF, left.values);
  const auto left_pixel_count = set1(kVHF, left.pixelCount());
  const auto left_average = div(kVHF, left_values, left_pixel_count);
  const auto left_plus = add(kVHF, whole_average, left_average);
  const auto left_minus = sub(kVHF, whole_average, left_average);
  const auto left_a = mul(kVHF, left_pixel_count, left_plus);
  const auto left_b = mul(kVHF, k2, left_values);
  const auto left_sum = mul(kVHF, left_minus, sub(kVHF, left_a, left_b));

  const auto right_values = load(kVHF, right.values);
  const auto right_pixel_count = set1(kVHF, right.pixelCount());
  const auto right_average = div(kVHF, right_values, right_pixel_count);
  const auto right_plus = add(kVHF, whole_average, right_average);
  const auto right_minus = sub(kVHF, whole_average, right_average);
  const auto right_a = mul(kVHF, right_pixel_count, right_plus);
  const auto right_b = mul(kVHF, k2, right_values);
  const auto right_sum = mul(kVHF, right_minus, sub(kVHF, right_a, right_b));

  auto sum = _mm_hadd_ps(left_sum, right_sum);
  sum = _mm_hadd_ps(sum, sum);
  sum = _mm_hadd_ps(sum, sum);

  float result;
  _mm_store_ss(&result, sum);

  return result;
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

class Fragment {
 public:
  Owned<Vector<int32_t>> region;
  std::unique_ptr<Fragment> left_child;
  std::unique_ptr<Fragment> right_child;

  Stats stats;
  Stats stats2;

  // Subdivision.
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

  void INLINE encode(RangeEncoder* dst, const CodecParams& cp, bool is_leaf,
                     std::vector<Fragment*>* children) {
    if (is_leaf) {
      RangeEncoder::writeNumber(dst, NodeType::COUNT, NodeType::FILL);
      for (size_t c = 0; c < 3; ++c) {
        int32_t q = cp.color_quant;
        int32_t d = 255 * stats.pixelCount();
        int32_t v = (2 * (q - 1) * stats.rgb(c) + d) / (2 * d);
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
    stats.sub(stats_cache.plus, stats_cache.minus);

    // Find subdivision
    for (int32_t angle_code = 0; angle_code < angle_max; ++angle_code) {
      int32_t angle = angle_code * angle_mult;
      DistanceRange distance_range;
      distance_range.update(region, angle, cp);
      int32_t num_lines = distance_range.num_lines;
      cache_stats[0].zero();
      for (int32_t line = 0; line < num_lines; ++line) {
        cache_stats[line + 1].updateGe(cache, angle,
                                       distance_range.distance(line));
        StatsCache::sum<true>(cache, stats_cache.x->data(), &stats_cache.minus);
        cache_stats[line + 1].sub(stats_cache.plus, stats_cache.minus);
      }
      cache_stats[num_lines + 1].copy(stats);

      for (int32_t line = 0; line < num_lines; ++line) {
        float full_score = 0.0f;
        constexpr const float kLocalWeight = 0.0f;
        if (kLocalWeight > 0.0f) {
          Stats band;
          band.sub(cache_stats[line + 2], cache_stats[line]);
          Stats left;
          left.sub(band, cache_stats[line + 1]);
          Stats right;
          right.sub(band, left);
          full_score += kLocalWeight * (score(band, left, right));
        }

        {
          Stats right;
          right.sub(stats, cache_stats[line + 1]);
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
  float tax =
      bitCost(NodeType::COUNT) + 3.0f * bitCost(CodecParams::makeColorQuant(0));
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
      result.push_back(candidate);
      candidate->left_child->findBestSubdivision(cache, cp);
      queue.push(candidate->left_child.get());
      candidate->right_child->findBestSubdivision(cache, cp);
      queue.push(candidate->right_child.get());
    }
  }
  return result;
}

static INLINE std::unordered_set<Fragment*> subpartition(
    int32_t target_size, const std::vector<Fragment*>& partition,
    CodecParams cp) {
  float tax = bitCost(NodeType::COUNT * cp.color_quant * cp.color_quant *
                      cp.color_quant);
  float budget = 8.0f * target_size - bitCost(CodecParams::kTax) - tax;
  budget -= sizeCost(cp.width);
  budget -= sizeCost(cp.height);
  std::unordered_set<Fragment*> non_leaf;
  for (Fragment* node : partition) {
    if (node->best_cost < 0.0f) break;
    float cost = node->best_cost + tax;
    if (budget < cost) break;
    budget -= cost;
    non_leaf.insert(node);
  }
  return non_leaf;
}

SIMD NOINLINE static float simulateEncode(int32_t target_size,
                                          std::vector<Fragment*>& partition,
                                          const CodecParams& cp) {
  std::unordered_set<Fragment*> non_leaf =
      subpartition(target_size, partition, cp);
  size_t leaf_count = non_leaf.size() + 1;
  Owned<Vector<float>> storage = allocVector(kVHF, 4 * leaf_count);
  float* RESTRICT stats = storage->data();
  size_t n = 0;
  for (Fragment* node : non_leaf) {
    Fragment* left = node->left_child.get();
    if (non_leaf.find(left) == non_leaf.end()) {
      store(load(kVHF, left->stats.values), kVHF, stats + 4 * n);
      n++;
    }
    Fragment* right = node->right_child.get();
    if (non_leaf.find(right) == non_leaf.end()) {
      store(load(kVHF, right->stats.values), kVHF, stats + 4 * n);
      n++;
    }
  }

  // sum((vi - v)^2) == sum(vi^2 - 2 * vi * v + v^2)
  //                 == n * v^2 + sum(vi^2) - 2 * v * sum(vi)

  int32_t v_max = cp.color_quant - 1;
  const auto quant = set1(kVHF, v_max / 255.0f);
  const auto dequant = set1(kVHF, 255.0f / v_max);
  const auto k2 = set1(kVHF, 2.0f);
  auto result_rgbx = zero(kVHF);
  for (size_t i = 0; i < n; ++i) {
    const auto rgbc = load(kVHF, stats + 4 * i);
    // a0 a1 a2 a3 $ a0 a1 a2 a3 -> a3 a3 a3 a3
    const auto pixel_count = _mm_shuffle_ps(rgbc, rgbc, 0xFF);
    const auto rgb_avg = div(kVHF, rgbc, pixel_count);
    const auto v_scaled = mul(kVHF, quant, rgb_avg);
    const auto v = round(kVHF, v_scaled);
    const auto v_dequant = mul(kVHF, v, dequant);
    const auto t0 = mul(kVHF, rgb_avg, k2);
    const auto t1 = mul(kVHF, pixel_count, v_dequant);
    const auto t2 = ::twim::sub(kVHF, v_dequant, t0);
    const auto t3 = mul(kVHF, t1, t2);
    result_rgbx = add(kVHF, result_rgbx, t3);
  }
  ALIGNED_16 float result_rgb[4];
  store(result_rgbx, kVHF, result_rgb);
  float result = result_rgbx[0] + result_rgbx[1] + result_rgbx[2];
  // result += SUM(rgb2)
  return result;
}

static std::vector<uint8_t> doEncode(int32_t target_size,
                                     const std::vector<Fragment*>& partition,
                                     const CodecParams& cp) {
  std::unordered_set<Fragment*> non_leaf =
      subpartition(target_size, partition, cp);
  std::vector<Fragment*> queue;
  queue.reserve(2 * non_leaf.size() + 1);
  queue.push_back(partition[0]);

  RangeEncoder dst;
  cp.write(&dst);

  size_t encoded = 0;
  while (encoded < queue.size()) {
    Fragment* node = queue[encoded++];
    node->encode(&dst, cp, non_leaf.find(node) == non_leaf.end(), &queue);
  }

  return dst.finish();
}

class SimulationTask {
 public:
  const int32_t target_size;
  const UberCache* uber;
  CodecParams cp;
  float best_sqe = 1e20f;
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

}  // namespace

std::vector<uint8_t> Encoder::encode(const Image& src, int32_t target_size) {
  int32_t width = src.width;
  int32_t height = src.height;
  if (width < 8 || height < 8) {
    fprintf(stderr, "image is too small (%d x %d)", width, height);
    return {};
  }
  UberCache uber(src);
  std::vector<SimulationTask> tasks;
  size_t max_tasks =
      CodecParams::kMaxLineLimit * CodecParams::kMaxPartitionCode;
  tasks.reserve(max_tasks);
  std::vector<std::future<void>> futures;
  futures.reserve(max_tasks);
  for (int32_t line_limit = 0; line_limit < CodecParams::kMaxLineLimit;
       ++line_limit) {
    for (int32_t partition_code = 0;
         partition_code < CodecParams::kMaxPartitionCode; ++partition_code) {
      // if (line_limit != 17) continue;
      // if (partition_code != 0) continue;
      tasks.emplace_back(target_size, &uber);
      SimulationTask& task = tasks.back();
      task.cp.setPartitionCode(partition_code);
      task.cp.line_limit = line_limit + 1;
      futures.push_back(
          std::async(std::launch::async, &SimulationTask::run, &task));
    }
  }
  auto t0 = now();
  size_t best_task_index = 0;
  float best_sqe = 1e20f;
  for (size_t task_index = 0; task_index < tasks.size(); ++task_index) {
    futures[task_index].get();
    const SimulationTask& task = tasks[task_index];
    if (task.best_sqe < best_sqe) {
      best_task_index = task_index;
      best_sqe = task.best_sqe;
    }
  }
  best_sqe += uber.rgb2[0] + uber.rgb2[1] + uber.rgb2[2];
  SimulationTask& best_task = tasks[best_task_index];
  best_task.cp.setColorCode(best_task.best_color_code);
  Fragment root = makeRoot(width, height);
  Cache cache(uber);
  std::vector<Fragment*> partition =
      buildPartition(&root, target_size, best_task.cp, &cache);
  std::vector<uint8_t> result = doEncode(target_size, partition, best_task.cp);
  auto t1 = now();

  std::string best_cp = best_task.cp.toString();
  fprintf(stdout, "time: %.3fs, size: %zu, cp: [%s], error: %.3f\n",
          duration(t0, t1), result.size(), best_cp.c_str(),
          std::sqrt(best_sqe / (width * height)));
  return result;
}

}  // namespace twim
