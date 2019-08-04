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

class Stats {
 public:
  // r, g, b, pixel_count, r2, g2, b2, reserved
  ALIGNED float values[8];

  INLINE float r() const { return values[0]; }
  INLINE float g() const { return values[1]; }
  INLINE float b() const { return values[2]; }
  INLINE float rgb(size_t c) const { return values[c]; }
  INLINE float pixelCount() const { return values[3]; }
  INLINE float r2() const { return values[4]; }
  INLINE float g2() const { return values[5]; }
  INLINE float b2() const { return values[6]; }
  INLINE float rgb2(size_t c) const { return values[c + 4]; }

  INLINE void setEmpty() {
    for (size_t i = 0; i < 8; ++i) values[i] = 0.0f;
  }

  void delta(Cache* cache, int32_t* RESTRICT region_x);

  // Calculate stats from own region.
  void update(Cache* cache);

  void INLINE copy(const Stats& other) {
    for (size_t i = 0; i < 8; ++i) values[i] = other.values[i];
  }

  void INLINE sub(const Stats& other) {
    for (size_t i = 0; i < 8; ++i) values[i] -= other.values[i];
  }

  void INLINE update(const Stats& parent, const Stats& sibling) {
    copy(parent);
    sub(sibling);
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
      : row_offset(allocVector<int32_t>(vecSize<int32_t>(capacity))),
        y(allocVector<float>(vecSize<float>(capacity))),
        x0(allocVector<int32_t>(vecSize<int32_t>(capacity))),
        x1(allocVector<int32_t>(vecSize<int32_t>(capacity))),
        x(allocVector<int32_t>(vecSize<int32_t>(capacity))) {}

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

  UberCache(const Image& src)
      : width(src.width),
        height(src.height),
        stride(8 * (src.width + 1)),
        sum(allocVector<float>(stride * src.height)) {
    float* RESTRICT sum = this->sum->data();
    for (size_t y = 0; y < src.height; ++y) {
      size_t src_row_offset = y * src.width;
      const uint8_t* RESTRICT r_row = src.r.data() + src_row_offset;
      const uint8_t* RESTRICT g_row = src.g.data() + src_row_offset;
      const uint8_t* RESTRICT b_row = src.b.data() + src_row_offset;
      size_t dstRowOffset = y * stride;
      for (size_t i = 0; i < 8; ++i) sum[dstRowOffset + i] = 0.0f;
      for (size_t x = 0; x < src.width; ++x) {
        size_t dstOffset = dstRowOffset + 8 * x;
        float r = r_row[x];
        float g = g_row[x];
        float b = b_row[x];
        sum[dstOffset +  8] = sum[dstOffset + 0] + r;
        sum[dstOffset +  9] = sum[dstOffset + 1] + g;
        sum[dstOffset + 10] = sum[dstOffset + 2] + b;
        sum[dstOffset + 11] = sum[dstOffset + 3] + 1.0f;
        sum[dstOffset + 12] = sum[dstOffset + 4] + r * r;
        sum[dstOffset + 13] = sum[dstOffset + 5] + g * g;
        sum[dstOffset + 14] = sum[dstOffset + 6] + b * b;
        sum[dstOffset + 15] = 0.0f;
      }
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

void INLINE SIMD StatsCache::sum(Cache* cache, int32_t* RESTRICT region_x,
                            Stats* dst) {
  StatsCache& stats_cache = cache->stats_cache;
  size_t count = stats_cache.count;
  const int32_t* RESTRICT rowOffset = stats_cache.row_offset->data();
  const float* RESTRICT sum = cache->uber->sum->data();

  constexpr auto vf = VecTag<float>();
  if (vf.N == 8) {
    auto tmp = zero(vf);
    for (size_t i = 0; i < count; ++i) {
      tmp = add(vf, tmp, load(vf, sum + rowOffset[i] + 8 * region_x[i]));
    }
    store(tmp, vf, dst->values);
  } else {
    Stats tmp;
    tmp.setEmpty();
    for (size_t i = 0; i < count; ++i) {
      int32_t offset = rowOffset[i] + 8 * region_x[i];
      for (size_t j = 0; j < 8; ++j) tmp.values[j] += sum[offset + j];
    }
    dst->copy(tmp);
  }
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
  this->count = count;
  sum(cache, x1, &plus);
}

void INLINE SIMD Stats::delta(Cache* cache, int32_t* RESTRICT region_x) {
  StatsCache& stats_cache = cache->stats_cache;
  Stats& minus = stats_cache.minus;
  StatsCache::sum(cache, region_x, &minus);

  Stats& plus = stats_cache.plus;
  for (size_t i = 0; i < 8; ++i) values[i] = plus.values[i] - minus.values[i];
}

// Calculate stats from own region.
void INLINE Stats::update(Cache* cache) {
  delta(cache, cache->stats_cache.x0->data());
}

void INLINE Stats::updateGeHorizontal(Cache* cache, int32_t d) {
  int32_t ny = SinCos::kCos[0];
  float dny = d / (float)ny;
  StatsCache& stats_cache = cache->stats_cache;
  size_t count = stats_cache.count;
  float* RESTRICT region_y = stats_cache.y->data();
  int32_t* RESTRICT region_x0 = stats_cache.x0->data();
  int32_t* RESTRICT region_x1 = stats_cache.x1->data();
  int32_t* RESTRICT region_x = stats_cache.x->data();

  for (size_t i = 0; i < count; i++) {
    float y = region_y[i];
    int32_t x0 = region_x0[i];
    int32_t x1 = region_x1[i];
    region_x[i] = (y < dny) ? x1 : x0;
  }
}

void INLINE Stats::updateGeGeneric(Cache* cache, int32_t angle, int32_t d) {
  int32_t nx = SinCos::kSin[angle];
  int32_t ny = SinCos::kCos[angle];
  float dnx = (2 * d + nx) / (float)(2 * nx);
  float mnynx = -(2 * ny) / (float)(2 * nx);
  StatsCache& stats_cache = cache->stats_cache;
  size_t count = stats_cache.count;
  float* RESTRICT region_y = stats_cache.y->data();
  int32_t* RESTRICT region_x0 = stats_cache.x0->data();
  int32_t* RESTRICT region_x1 = stats_cache.x1->data();
  int32_t* RESTRICT region_x = stats_cache.x->data();

  for (size_t i = 0; i < count; i++) {
    float y = region_y[i];
    int32_t xf = static_cast<int32_t>(y * mnynx + dnx);  // FMA
    int32_t x0 = region_x0[i];
    int32_t x1 = region_x1[i];
    region_x[i] = (x1 > xf) ? ((xf > x0) ? xf : x0) : x1;
  }
}

void INLINE Stats::updateGe(Cache* cache, int angle, int d) {
  if (angle == 0) {
    updateGeHorizontal(cache, d);
  } else {
    updateGeGeneric(cache, angle, d);
  }
  delta(cache, cache->stats_cache.x->data());
}

static INLINE float score(const Stats& whole, const Stats& part) {
  if (part.pixelCount() <= 0.0f) return 0.0f;
  float result = 0.0f;
  const float inv_whole_pixel_count = 1.0f / whole.pixelCount();
  const float inv_part_pixel_count = 1.0f / part.pixelCount();
  for (size_t c = 0; c < 3; ++c) {
    float whole_average = whole.rgb(c) * inv_whole_pixel_count;
    float part_average = part.rgb(c) * inv_part_pixel_count;
    float plus = whole_average + part_average;
    float minus = whole_average - part_average;
    result += minus * (part.pixelCount() * plus - 2.0f * part.rgb(c));
  }
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

  float INLINE simulateEncode(CodecParams cp, bool is_leaf,
                       std::vector<Fragment*>* children) {
    float result = 0.0f;
    if (is_leaf) {
      for (size_t c = 0; c < 3; ++c) {
        int32_t q = cp.color_quant;
        int32_t d = 255 * stats.pixelCount();
        int32_t v = (2 * (q - 1) * stats.rgb(c) + d) / (2 * d);
        int32_t vq = CodecParams::dequantizeColor(v, q);
        // sum((vi - v)^2) == sum(vi^2 - 2 * vi * v + v^2)
        //                 == n * v^2 + sum(vi^2) - 2 * v * sum(vi)
        result += stats.pixelCount() * vq * vq + stats.rgb2(c) -
                  2 * vq * stats.rgb(c);
      }
      return result;
    }
    children->push_back(left_child.get());
    children->push_back(right_child.get());
    return 0.0f;
  }

  void INLINE findBestSubdivision(Cache* cache, CodecParams cp) {
    Vector<int32_t>& region = *this->region.get();
    Stats& stats = this->stats;
    Stats* cache_stats = cache->stats;
    int32_t level = cp.getLevel(region);
    int32_t angle_max = 1 << cp.angle_bits[level];
    int32_t angle_mult = (SinCos::kMaxAngle / angle_max);
    int32_t best_angle_code = 0;
    int32_t best_line = 0;
    float best_score = -1.0f;
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
        float full_score = 0.0f;
        constexpr const float kLocalWeight = 0.0f;
        if (kLocalWeight > 0.0f) {
          Stats band;
          band.update(cache_stats[line + 2], cache_stats[line]);
          Stats left;
          left.update(band, cache_stats[line + 1]);
          Stats right;
          right.update(band, left);
          full_score += 0.0f * (score(band, left) + score(band, right));
        }

        {
          Stats right;
          right.update(stats, cache_stats[line + 1]);
          full_score += 1.0f * (score(stats, cache_stats[line + 1]) +
                                score(stats, right));
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
      int32_t child_step = vecSize<int32_t>(region.len);
      this->left_child.reset(
          new Fragment(allocVector<int32_t>(3 * child_step)));
      this->right_child.reset(
          new Fragment(allocVector<int32_t>(3 * child_step)));
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
  int32_t step = vecSize<int32_t>(height);
  Owned<Vector<int32_t>> root = allocVector<int32_t>(3 * step);
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
static INLINE std::vector<Fragment*> buildPartition(Fragment* root, size_t size_limit,
                                             const CodecParams& cp,
                                             Cache* cache) {
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

static float INLINE simulateEncode(int32_t target_size,
                            std::vector<Fragment*>& partition,
                            const CodecParams& cp) {
  std::unordered_set<Fragment*> non_leaf =
      subpartition(target_size, partition, cp);
  std::vector<Fragment*> queue;
  queue.reserve(2 * non_leaf.size() + 1);
  queue.push_back(partition[0]);

  float result = 0.0f;
  size_t encoded = 0;
  while (encoded < queue.size()) {
    Fragment* node = queue[encoded++];
    result +=
        node->simulateEncode(cp, non_leaf.find(node) == non_leaf.end(), &queue);
  }

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
    // if (line_limit != 17) continue;
    for (int32_t partition_code = 0;
         partition_code < CodecParams::kMaxPartitionCode; ++partition_code) {
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
