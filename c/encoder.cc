//#include "encoder.h"

#include "encoder.h"

#include <hwy/highway.h>

#if !defined(__EMSCRIPTEN__)
#include <future>
#endif

#include <memory>
#include <queue>

#include "codec_params.h"
#include "encoder_internal.h"
#include "encoder_simd.h"
#include "image.h"
#include "platform.h"
#include "xrange_encoder.h"

namespace twim {

using ::twim::Encoder::Result;
using ::twim::Encoder::Variant;

#if defined(__EMSCRIPTEN__)
static void log(const char* message) {(void)message;}
#else
static void log(const char* message) {
  fprintf(stderr, "%s\n", message);
}
#endif

void writeSize(XRangeEncoder* dst, uint32_t value) {
  value -= 8;
  uint32_t chunks = 2;
  while (value > (1u << (chunks * 3))) {
    value -= (1u << (chunks * 3));
    chunks++;
  }
  for (uint32_t i = 0; i < chunks; ++i) {
    if (i > 1) {
      XRangeEncoder::writeNumber(dst, 2, 1);
    }
    XRangeEncoder::writeNumber(dst, 8, (value >> (3 * (chunks - i - 1))) & 7u);
  }
  XRangeEncoder::writeNumber(dst, 2, 0);
}

void CodecParams::write(XRangeEncoder* dst) const {
  writeSize(dst, width);
  writeSize(dst, height);
  XRangeEncoder::writeNumber(dst, kMaxF1, params[0]);
  XRangeEncoder::writeNumber(dst, kMaxF2, params[1]);
  XRangeEncoder::writeNumber(dst, kMaxF3, params[2]);
  XRangeEncoder::writeNumber(dst, kMaxF4, params[3]);
  XRangeEncoder::writeNumber(dst, kMaxLineLimit, line_limit - 1);
  XRangeEncoder::writeNumber(dst, kMaxColorCode, color_code);
}

NOINLINE double CodecParams::getTax() const {
  XRangeEncoder fakeEncoder;
  write(&fakeEncoder);
  return XRangeEncoder::estimateCost(&fakeEncoder);
}

UberCache::UberCache(const Image& src)
    : width(src.width),
      height(src.height),
      // 4 == [r, g, b, count].length
      stride(vecSize(4 * (src.width + 1))),
      sum(allocVector<float>(stride * src.height)) {
  float* RESTRICT sum = this->sum->data();
  for (size_t y = 0; y < src.height; ++y) {
    float row_rgb2[3] = {0.0f};
    size_t src_row_offset = y * src.width;
    const uint8_t* RESTRICT r_row = src.r + src_row_offset;
    const uint8_t* RESTRICT g_row = src.g + src_row_offset;
    const uint8_t* RESTRICT b_row = src.b + src_row_offset;
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

Cache::Cache(const UberCache& uber)
    : uber(&uber),
      row_offset(allocVector<int32_t>(uber.height)),
      y(allocVector<float>(uber.height)),
      x0(allocVector<int32_t>(uber.height)),
      x1(allocVector<int32_t>(uber.height)),
      x(allocVector<int32_t>(uber.height)),
      stats(allocVector<float>(
          6 * vecSize(CodecParams::kMaxLineLimit * SinCos.kMaxAngle))) {}

class SimulationTask {
 public:
  const uint32_t targetSize;
  Variant variant;
  CodecParams cp;
  float bestSqe = 1e35f;
  uint32_t bestColorCode = (uint32_t)-1;
  Partition* partitionHolder = nullptr;

  SimulationTask(uint32_t targetSize, Variant variant, const UberCache& uber)
      : targetSize(targetSize), variant(variant), cp(uber.width, uber.height) {}

  ~SimulationTask() {
    delete partitionHolder;
  }

  void run(Cache* cache) {
    cp.setPartitionCode(variant.partitionCode);
    cp.line_limit = variant.lineLimit + 1;
    uint64_t colorOptions = variant.colorOptions;
    // TODO: color-options based taxes
    partitionHolder = new Partition(cache, cp, targetSize);
    for (uint32_t colorCode = 0; colorCode < CodecParams::kMaxColorCode;
         ++colorCode) {
      if (!(colorOptions & (1 << colorCode))) continue;
      cp.setColorCode(colorCode);
      float sqe = simulateEncode(*partitionHolder, targetSize, cp);
      if (sqe < bestSqe) {
        bestSqe = sqe;
        bestColorCode = colorCode;
      }
    }
  }
};

class TaskExecutor {
 public:
  explicit TaskExecutor(const UberCache& uber, size_t max_tasks) : uber(&uber) {
    tasks.reserve(max_tasks);
  }

  void run() {
    float bestSqe = 1e35f;
    size_t lastBestTask = (size_t)-1;
    Cache cache(*uber);

    while (true) {
      size_t myTask = nextTask++;
      if (myTask >= tasks.size()) return;
      SimulationTask& task = tasks[myTask];
      task.run(&cache);
      if (task.bestSqe < bestSqe) {
        bestSqe = task.bestSqe;
        if (lastBestTask < myTask) {
          delete tasks[lastBestTask].partitionHolder;
          tasks[lastBestTask].partitionHolder = nullptr;
        }
        lastBestTask = myTask;
      } else {
        delete task.partitionHolder;
        task.partitionHolder = nullptr;
      }
    }
  }

  const UberCache* uber;
  std::atomic<size_t> nextTask{0};
  std::vector<SimulationTask> tasks;
};

NOINLINE void Fragment::encode(XRangeEncoder* dst, const CodecParams& cp,
                               bool is_leaf, const float* RESTRICT palette,
                               std::vector<Fragment*>* children) {
  if (is_leaf) {
    XRangeEncoder::writeNumber(dst, NodeType::COUNT, NodeType::FILL);
    float inv_c = 1.0f / stats[3];
    if (cp.palette_size == 0) {
      float quant = static_cast<float>(cp.color_quant - 1) / 255.0f;
      for (size_t c = 0; c < 3; ++c) {
        uint32_t v = static_cast<uint32_t>(quant * stats[c] * inv_c + 0.5f);
        XRangeEncoder::writeNumber(dst, cp.color_quant, v);
      }
    } else {
      float r = stats[0] * inv_c;
      float g = stats[1] * inv_c;
      float b = stats[2] * inv_c;
      size_t palette_step = vecSize(cp.palette_size);
      float dummy;
      uint32_t index =
          chooseColor(r, g, b, palette, palette + palette_step,
                      palette + 2 * palette_step, cp.palette_size, &dummy);
      XRangeEncoder::writeNumber(dst, cp.palette_size, index);
    }
    return;
  }
  XRangeEncoder::writeNumber(dst, NodeType::COUNT, NodeType::HALF_PLANE);
  uint32_t maxAngle = 1u << cp.angle_bits[level];
  XRangeEncoder::writeNumber(dst, maxAngle, best_angle_code);
  XRangeEncoder::writeNumber(dst, best_num_lines, best_line);
  children->push_back(leftChild);
  children->push_back(rightChild);
}

std::vector<uint8_t> doEncode(uint32_t num_non_leaf,
                              const std::vector<Fragment*>* partition,
                              const CodecParams& cp,
                              const float* RESTRICT palette) {
  const uint32_t m = cp.palette_size;
  uint32_t n = num_non_leaf + 1;

  std::vector<Fragment*> queue;
  queue.reserve(n);
  queue.push_back(partition->at(0));

  XRangeEncoder dst;
  cp.write(&dst);

  size_t palette_step = vecSize(cp.palette_size);

  for (uint32_t j = 0; j < m; ++j) {
    for (size_t c = 0; c < 3; ++c) {
      uint32_t clr = static_cast<uint32_t>(palette[c * palette_step + j]);
      XRangeEncoder::writeNumber(&dst, 256, clr);
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

struct PqNode {
  PqNode(Fragment* v = nullptr) : v(v) {}

  Fragment* v;
  uint32_t leftChild = 0;
  uint32_t nextSibling = 0;
};

void addChild(PqNode* storage, uint32_t node, uint32_t child) {
  uint32_t leftChild = storage[node].leftChild;
  if (leftChild != 0) {
    storage[child].nextSibling = leftChild;
  }
  storage[node].leftChild = child;
}

NOINLINE uint32_t merge(PqNode* storage, uint32_t a, uint32_t b) {
  if (a == 0) return b;
  if (b == 0) return a;

  if (storage[a].v->best_score < storage[b].v->best_score) {
    uint32_t c = a;
    a = b;
    b = c;
  }

  addChild(storage, a, b);
  return a;
}

// TODO(eustas): turn to loop
uint32_t fold(PqNode* storage, uint32_t node) {
  if (node == 0) return 0;
  uint32_t sibling = storage[node].nextSibling;
  if (sibling == 0) return node;
  uint32_t tail = storage[sibling].nextSibling;
  storage[node].nextSibling = 0;
  storage[sibling].nextSibling = 0;
  return merge(storage, merge(storage, node, sibling), fold(storage, tail));
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
  float tax = SinCos.kLog2[NodeType::COUNT];
  float budget =
      size_limit * 8.0f - tax - cp.getTax();  // Minus flat image cost.

  std::vector<Fragment*> result;

  findBestSubdivision(root, cache, cp);

  std::vector<PqNode> queue;
  // 0-th node is a "nullptr"
  queue.emplace_back(nullptr);
  queue.emplace_back(root);
  uint32_t rootNode = 1;
  uint32_t next = 2;

  while (rootNode != 0) {
    Fragment* candidate = queue[rootNode].v;
    rootNode = fold(queue.data(), queue[rootNode].leftChild);  // pop
    if (candidate->best_score < 0.0 || candidate->best_cost < 0.0) break;
    if (tax + candidate->best_cost <= budget) {
      budget -= tax + candidate->best_cost;
      candidate->ordinal = static_cast<uint32_t>(result.size());
      result.push_back(candidate);
      findBestSubdivision(candidate->leftChild, cache, cp);
      queue.emplace_back(candidate->leftChild);
      rootNode = merge(queue.data(), rootNode, next++);  // push
      findBestSubdivision(candidate->rightChild, cache, cp);
      queue.emplace_back(candidate->rightChild);
      rootNode = merge(queue.data(), rootNode, next++);  // push
    }
  }
  return result;
}

void initRoot(Fragment* root, uint32_t width, uint32_t height) {
  uint32_t step = vecSize(height);
  int32_t* RESTRICT data = root->region->data();
  for (uint32_t y = 0; y < height; ++y) {
    data[y] = y;
    data[step + y] = 0;
    data[2 * step + y] = width;
  }
  root->region->len = height;
}

Partition::Partition(Cache* cache, const CodecParams& cp, size_t target_size)
    : root(cache->uber->height) {
  initRoot(&root, cache->uber->width, cache->uber->height);
  partition = buildPartition(&root, target_size, cp, cache);
}

const std::vector<Fragment*>* Partition::getPartition() const {
  return &partition;
}

/* Returns the index of the first leaf node in partition. */
uint32_t Partition::subpartition(const CodecParams& cp,
                                 uint32_t target_size) const {
  const std::vector<Fragment*>* partition = getPartition();
  float node_tax = SinCos.kLog2[NodeType::COUNT];
  float image_tax = cp.getTax();
  if (cp.palette_size == 0) {
    node_tax += 3.0f * SinCos.kLog2[cp.color_quant];
  } else {
    node_tax += SinCos.kLog2[cp.palette_size];
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

Result encode(const Image& src, const Params& params) {
  Result result{};

  int32_t width = src.width;
  int32_t height = src.height;
  if (width < 8 || height < 8) {
    if (params.debug) log("image is too small");
    return result;
  }
  UberCache uber(src);
  const Variant* variants = params.variants;
  size_t numVariants = params.numVariants;
  if (numVariants == 0) {
    if (params.debug) log("no encoding variants specified");
    return result;
  }

#if defined(__EMSCRIPTEN__)
  SimulationTask bestTask(params.targetSize, variants[0], uber);
  Cache cache(uber);
  bestTask.run(&cache);
  float bestSqe = bestTask.bestSqe;
#else
  TaskExecutor executor(uber, numVariants);
  std::vector<SimulationTask>& tasks = executor.tasks;
  for (size_t i = 0; i < numVariants; ++i) {
    tasks.emplace_back(params.targetSize, variants[i], uber);
  }
  std::vector<std::future<void>> futures;
  size_t numThreads = std::min<size_t>(params.numThreads, numVariants);
  futures.reserve(numThreads);
  bool singleThreaded = (numThreads == 1);
  for (uint32_t i = 0; i < numThreads; ++i) {
    futures.push_back(
        std::async(singleThreaded ? std::launch::deferred : std::launch::async,
                   &TaskExecutor::run, &executor));
  }
  for (uint32_t i = 0; i < numThreads; ++i) futures[i].get();
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
  SimulationTask& bestTask = tasks[bestTaskIndex];
#endif

  CodecParams& cp = bestTask.cp;
  uint32_t bestColorCode = bestTask.bestColorCode;
  cp.setColorCode(bestColorCode);
  const Partition& partitionHolder = *bestTask.partitionHolder;

  // Encoder workflow >>
  // Partition partitionHolder(uber, cp, params.targetSize);
  uint32_t numNonLeaf = partitionHolder.subpartition(cp, params.targetSize);
  const std::vector<Fragment*>* partition = partitionHolder.getPartition();
  std::unique_ptr<Vector<float>> patches = gatherPatches(partition, numNonLeaf);
  std::unique_ptr<Vector<float>> palette =
      buildPalette(patches, cp.palette_size);
  float* RESTRICT colors = palette->data();
  result.data = doEncode(numNonLeaf, partition, cp, colors);
  // << Encoder workflow

  result.variant = bestTask.variant;
  result.variant.colorOptions = (uint64_t)1 << bestColorCode;
  result.mse = (bestSqe + uber.rgb2[0] + uber.rgb2[1] + uber.rgb2[2]) /
               static_cast<float>(width * height);
  return result;
}

}  // namespace Encoder

}  // namespace twim
