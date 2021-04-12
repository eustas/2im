package ru.eustas.twim;

import static ru.eustas.twim.XRangeEncoder.writeNumber;

import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.PriorityQueue;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicInteger;

public class Encoder {
  private static final EncoderCore CORE;
  private static final int CORE_PADDING;

  static {
    EncoderCore core = new EncoderVanillaCore();
    try {
      core = (EncoderCore) Class.forName("ru.eustas.twim.EncoderSimdCore").getDeclaredConstructor().newInstance();
    } catch (Throwable ex) {
      // That's ok.
    }
    CORE = core;
    CORE_PADDING = CORE.padding();
  }

  private Encoder() {}

  public static class Variant {
    int partitionCode;
    int lineLimit;
    long colorOptions;
  }

  public static class Params {
    int targetSize = 287;
    int numThreads = 1;
    ArrayList<Variant> variants = new ArrayList<>();
    boolean debug = false;
  }

  public static class Result {
    byte[] data;
    Variant variant;
    float mse;
  }

  static class UberCache {
    final int width;
    final int height;
    /* Cumulative sum of previous row values. Extra column with total sum. */
    final int[] sum;
    final int stride;

    final float sqeBase;
    private final float imageTax;

    UberCache(int[] intRgb, int width) {
      int height = intRgb.length / width;
      if (width * height != intRgb.length) {
        throw new IllegalArgumentException("width * height != length");
      }

      this.width = width;
      this.height = height;

      int stride = 4 * (width + 1 + 3);
      int[] sum = new int[stride * height];
      float sum2 = 0.0f;
      for (int y = 0; y < height; ++y) {
        int srcRowOffset = y * width;
        int dstRowOffset = y * stride;
        float rowSum2 = 0.0f;
        for (int x = 0; x < width; ++x) {
          int dstOffset = dstRowOffset + 4 * x;
          int rgbValue = intRgb[srcRowOffset + x];
          int r = (rgbValue >> 16) & 0xFF;
          int g = (rgbValue >> 8) & 0xFF;
          int b = rgbValue & 0xFF;
          sum[dstOffset + 4] = sum[dstOffset + 0] + r;
          sum[dstOffset + 5] = sum[dstOffset + 1] + g;
          sum[dstOffset + 6] = sum[dstOffset + 2] + b;
          sum[dstOffset + 7] = sum[dstOffset + 3] + 1;
          rowSum2 += r * r + g * g + b * b;
        }
        sum2 += rowSum2;
      }
      this.sum = sum;
      this.stride = stride;
      this.sqeBase = sum2;
      this.imageTax = CodecParams.calculateImageTax(width, height);
    }

    float bitBudget(int targetSize) {
      return 8.0f * targetSize - 4.0f - imageTax;
    }
  }

  private static class Patches {
    final float[] wr;
    final float[] wg;
    final float[] wb;
    final float[] c;
    final float[] r;
    final float[] g;
    final float[] b;
    Patches(int size) {
      wr = new float[size];
      wg = new float[size];
      wb = new float[size];
      c = new float[size];
      r = new float[size];
      g = new float[size];
      b = new float[size];
    }
  }

  static class Cache {
    final UberCache uber;

    final DistanceRange distanceRange = new DistanceRange();

    final int[] stats = new int[4];
    final int[] plus = new int[4];
    final int[] minus = new int[8];
    final float[] left = new float[4];

    final float[] orig = new float[3];
    final float[] rgb = new float[3];

    static final int STATS_SIZE = CodecParams.MAX_LINE_LIMIT * SinCos.MAX_ANGLE;
    float[] statsR = new float[STATS_SIZE + 15];
    float[] statsG = new float[STATS_SIZE + 15];
    float[] statsB = new float[STATS_SIZE + 15];
    float[] statsC = new float[STATS_SIZE + 15];
    float[] statsS = new float[STATS_SIZE + 15];
    int[] statsV = new int[STATS_SIZE + 15];

    int count;
    final int[] rowOffset;
    final float[] y;
    final int[] yi;
    final int[] x0;
    final int[] x1;
    final int[] x;
    final float[] x0f;
    final float[] x1f;
    final float[] xf;

    private Patches patches = new Patches(1);

    Cache(UberCache uber) {
      this.uber = uber;
      this.rowOffset = new int[uber.height + 15];
      this.y = new float[uber.height + 15];
      this.yi = new int[uber.height + 15];
      this.x0 = new int[uber.height + 15];
      this.x1 = new int[uber.height + 15];
      this.x = new int[uber.height + 15];
      this.x0f = new float[uber.height + 15];
      this.x1f = new float[uber.height + 15];
      this.xf = new float[uber.height + 15];
    }

    Patches getPatches(int n) {
      int len = patches.r.length;
      if (len < n) {
        n = Math.max(n, 2 * len);
        patches = new Patches(n);
      }
      return patches;
    }

    void prepare(int[] region) {
      int n = region[region.length - 1];
      int step = region.length / 3;
      if ((uber.stride & 3) != 0) throw new IllegalStateException();
      int stride = uber.stride / 4;
      for (int i = 0; i < n; ++i) {
        int row = region[i];
        yi[i] = row;
        y[i] = row;
        x0[i] = region[step + i];
        x0f[i] = region[step + i];
        x1[i] = region[2 * step + i];
        x1f[i] = region[2 * step + i];
        rowOffset[i] = row * stride;
      }
      for (int i = n; (i & (CORE_PADDING - 1)) != 0; i++) {
        yi[i] = 0;
        y[i] = 0;
        x0[i] = 0;
        x0f[i] = 0;
        x1[i] = 0;
        x1f[i] = 0;
        rowOffset[i] = 0;
      }
      this.count = n;
    }

    void sum(int[] regionX, int[] dst) {
      int count = this.count;
      int[] rowOffset = this.rowOffset;
      int[] sum = uber.sum;
      for (int j = 0; j < 4; ++j) dst[j] = 0;
      for (int i = 0; i < count; i++) {
        int offset = rowOffset[i] + regionX[i];
        for (int j = 0; j < 4; ++j) dst[j] += sum[(offset * 4) + j];
      }
    }

    // y * ny >= d
    void updateGeHorizontal(int d) {
      int ny = SinCos.COS[0];
      float dny = d / (float) ny;
      int count = this.count;
      int[] rowOffset = this.rowOffset;
      float[] regionY = this.y;
      int[] regionX0 = this.x0;
      int[] regionX1 = this.x1;
      int[] regionX = this.x;

      for (int i = 0; i < count; i++) {
        float y = regionY[i];
        int offset = rowOffset[i];
        int x0 = regionX0[i];
        int x1 = regionX1[i];
        int x = (y < dny) ? x1 : x0;
        int xOff = x + offset;
        regionX[i] = xOff;
      }
    }
  }

  private static int chooseMax(float[] a, int n) {
    int index = 0;
    float value = a[0];
    for (int i = 0; i < n; ++i) {
      if (a[i] > value) {
        index = i;
        value = a[i];
      }
    }
    return index;
  }

  static class Fragment {
    final int[] region;
    final float[] stats = new float[4];
    Fragment leftChild;
    Fragment rightChild;

    // Subdivision.
    int ordinal = 0x7FFFFFFF;
    int level;
    int bestAngleCode;
    int bestLine;
    float bestScore;
    int bestNumLines;
    float bestCost;

    private Fragment(int[] region) {
      this.region = region;
    }

    void findBestSubdivision(Cache cache, CodecParams cp) {
      int[] region = this.region;

      DistanceRange distanceRange = cache.distanceRange;
      int[] stats = cache.stats;
      int[] plus = cache.plus;
      int[] minus = cache.minus;
      float[] left = cache.left;
      float[] statsR = cache.statsR;
      float[] statsG = cache.statsG;
      float[] statsB = cache.statsB;
      float[] statsC = cache.statsC;
      float[] statsS = cache.statsS;
      int[] statsV = cache.statsV;

      int level = cp.getLevel(region);
      int angleMax = 1 << cp.angleBits[level];
      int angleMult = (SinCos.MAX_ANGLE / angleMax);
      cache.prepare(region);
      cache.sum(cache.x1, plus);
      cache.sum(cache.x0, minus);
      for (int i = 0; i < 4; ++i) stats[i] = plus[i] - minus[i];
      for (int i = 0; i < 4; ++i) this.stats[i] = stats[i];

      int numSubdivisions = 0;
      float minC = 0.5f;
      float maxC = stats[3] - 0.5f;

      // Find subdivision
      for (int angleCode = 0; angleCode < angleMax; ++angleCode) {
        int angle = angleCode * angleMult;
        // CPU: 6% vs 5.3% | 1073ms vs 504ms
        distanceRange.update(region, angle, cp);
        int numLines = distanceRange.numLines;
        for (int line = 0; line < numLines; ++line) {
          int distance = distanceRange.distance(line);
          if (angle == 0) {
            cache.updateGeHorizontal(distance);
          } else {
            // CPU: 12% vs 24.6% | 2.1s vs 3.29s
            CORE.updateGeGeneric(angle, distance, cache.y, cache.x0, cache.x0f, cache.x1, cache.x1f, cache.rowOffset, cache.x, cache.count, cache.uber.stride / 4);
          }
          // CPU: 59% vs 39.5% | 10.3s vs 5.29s
          CORE.sumAbs(cache.uber.sum, cache.count, cache.x, minus);
          for (int i = 0; i < 4; ++i) left[i] = plus[i] - minus[i];
          statsR[numSubdivisions] = left[0];
          statsG[numSubdivisions] = left[1];
          statsB[numSubdivisions] = left[2];
          statsC[numSubdivisions] = left[3];
          statsV[numSubdivisions] = line * angleMax + angleCode;
          float c = left[3];
          numSubdivisions += ((c > minC) && (c < maxC)) ? 1 : 0;
        }
      }

      if (numSubdivisions == 0) {
        this.bestCost = -1.0f;
        this.bestScore = -1.0f;
        return;
      }

      // CPU: 1% vs 1.5% | 229ms vs 200ms
      int tail = numSubdivisions;
      int mask = CORE_PADDING - 1;
      while ((tail & mask) != 0) {
        statsR[tail] = statsR[tail - 1];
        statsG[tail] = statsB[tail - 1];
        statsB[tail] = statsB[tail - 1];
        statsC[tail] = statsC[tail - 1];
        statsV[tail] = statsV[tail - 1];
        tail++;
      }
      CORE.score(stats, numSubdivisions, statsR, statsG, statsB, statsC, statsS);
      int index = chooseMax(statsS, numSubdivisions);
      int bestAngleCode = statsV[index] % angleMax;
      int bestLine = statsV[index] / angleMax;
      float bestScore = statsS[index];

      this.level = level;
      this.bestScore = bestScore;

      if (bestScore < 0.5f) {
        this.bestScore = -1.0f;
        this.bestCost = -1.0f;
        // TODO(eustas): why not unreachable?
      } else {
        distanceRange.update(region, bestAngleCode * angleMult, cp);
        int childStep = Region.vectorFriendlySize(region[region.length - 1]);
        int[] leftRegion = new int[childStep * 3 + 1];
        int[] rightRegion = new int[childStep * 3 + 1];
        Region.splitLine(region, bestAngleCode * angleMult, distanceRange.distance(bestLine), leftRegion, rightRegion);
        this.leftChild = new Fragment(leftRegion);
        this.rightChild = new Fragment(rightRegion);

        this.bestAngleCode = bestAngleCode;
        this.bestNumLines = distanceRange.numLines;
        this.bestLine = bestLine;
        this.bestCost = bitCost(CodecParams.NODE_TYPE_COUNT * angleMax * distanceRange.numLines);
      }
    }

    void encode(XRangeEncoder dst, CodecParams cp, boolean isLeaf, List<Fragment> children) {
      if (isLeaf) {
        writeNumber(dst, CodecParams.NODE_TYPE_COUNT, CodecParams.NODE_FILL);
        float invC = 1.0f / stats[3];
        if (cp.paletteSize == 0) {
          float quant = (cp.colorQuant - 1) / 255.0f;
          for (int c = 0; c < 3; ++c) {
            int v = (int)(quant * stats[c] * invC + 0.5f);
            writeNumber(dst, cp.colorQuant, v);
          }
        } else {
          throw new IllegalStateException();
        }
        return;
      }
      writeNumber(dst, CodecParams.NODE_TYPE_COUNT, CodecParams.NODE_HALF_PLANE);
      int maxAngle = 1 << cp.angleBits[level];
      writeNumber(dst, maxAngle, bestAngleCode);
      writeNumber(dst, bestNumLines, bestLine);
      children.add(leftChild);
      children.add(rightChild);
    }
  }

  private static float bitCost(int range) {
    return (float)(Math.log(range) / Math.log(2.0));
  }

  private static class FragmentComparator implements Comparator<Fragment> {
    @Override
    public int compare(Fragment o1, Fragment o2) {
      if (o1.bestScore == o2.bestScore) {
        return 0;
      }
      return o1.bestScore < o2.bestScore ? 1 : -1;
    }
  }
  private static final FragmentComparator FRAGMENT_COMPARATOR = new FragmentComparator();

  /**
   * Builds the space partition.
   *
   * Minimal color data cost is used.
   * Partition could be used to try multiple color quantizations to see, which one gives the best result.
   */
  static List<Fragment> buildPartition(float bitBudget, CodecParams cp, Cache cache) {
    float tax = bitCost(CodecParams.NODE_TYPE_COUNT);

    int width = cp.width;
    int height = cp.height;
    int step = Region.vectorFriendlySize(height);
    int[] rootRegion = new int[step * 3 + 1];
    for (int y = 0; y < height; ++y) {
      rootRegion[y] = y;
      rootRegion[step + y] = 0;
      rootRegion[2 * step + y] = width;
    }
    rootRegion[rootRegion.length - 1] = height;
    Fragment root = new Fragment(rootRegion);

    root.findBestSubdivision(cache, cp);

    List<Fragment> result = new ArrayList<>();
    PriorityQueue<Fragment> queue = new PriorityQueue<>(16, FRAGMENT_COMPARATOR);
    queue.add(root);

    while (!queue.isEmpty()) {
      Fragment candidate = queue.poll();
      if (candidate.bestScore < 0.0 || candidate.bestCost < 0.0) break;
      float cost = tax + candidate.bestCost;
      if (cost <= bitBudget) {
        bitBudget -= cost;
        candidate.ordinal = result.size();
        result.add(candidate);
        candidate.leftChild.findBestSubdivision(cache, cp);
        queue.add(candidate.leftChild);
        candidate.rightChild.findBestSubdivision(cache, cp);
        queue.add(candidate.rightChild);
      }
    }
    return result;
  }

  private static int subpartition(float bitBudget, List<Fragment> partition, CodecParams cp) {
    float nodeTax = 1.0f;  // log2(CodecParams.NODE_TYPE_COUNT)
    if (cp.paletteSize == 0) {
      nodeTax += 3.0f * bitCost(cp.colorQuant);
    } else {
      nodeTax += bitCost(cp.paletteSize);
      bitBudget -= 24.0f * cp.paletteSize;  // 3 x 8 bit
    }
    bitBudget -= nodeTax;
    int limit = partition.size();
    int i;
    for (i = 0; i < limit; ++i) {
      Fragment node = partition.get(i);
      if (node.bestCost < 0.0f) break;
      float cost = node.bestCost + nodeTax;
      if (bitBudget < cost)
        break;
      bitBudget -= cost;
    }
    return i;
  }

  private static void gatherPatches(List<Fragment> partition, int numNonLeaf, Patches patches) {
    int pos = 0;
    for (int i = 0; i < numNonLeaf; ++i) {
      Fragment node = partition.get(i);
      Fragment leaf = node.leftChild;
      if (leaf.ordinal >= numNonLeaf) {
        patches.wr[pos] = leaf.stats[0];
        patches.wg[pos] = leaf.stats[1];
        patches.wb[pos] = leaf.stats[2];
        patches.c[pos] = leaf.stats[3];
        pos++;
      }
      leaf = node.rightChild;
      if (leaf.ordinal >= numNonLeaf) {
        patches.wr[pos] = leaf.stats[0];
        patches.wg[pos] = leaf.stats[1];
        patches.wb[pos] = leaf.stats[2];
        patches.c[pos] = leaf.stats[3];
        pos++;
      }
    }

    // TODO(eustas): check if it gets properly vectorized.
    for (int i = 0; i < numNonLeaf + 1; i++) {
      float c = patches.c[i];
      float invC = 1.0f / c;
      patches.r[i] = patches.wr[i] * invC;
      patches.g[i] = patches.wg[i] * invC;
      patches.b[i] = patches.wb[i] * invC;
    }
  }

  private static float simulateEncode(float bitBudget, List<Fragment> partition, CodecParams cp, Cache cache) {
    int numNonLeaf = subpartition(bitBudget, partition, cp);
    if (numNonLeaf <= 1) {
      // Let's deal with flat image separately.
      return 1e35f;
    }
    int n = numNonLeaf + 1;
    int m = cp.paletteSize;
    Patches patches = cache.getPatches(n);
    gatherPatches(partition, numNonLeaf, patches);

    float result = 0.0f;
    int vMax = cp.colorQuant - 1;
    float quant = vMax / 255.0f;
    float dequant = 255.0f / ((vMax != 0) ? vMax : 1);

    float[] orig = cache.orig;
    float[] rgb = cache.rgb;
    for (int i = 0; i < n; ++i) {
      orig[0] = patches.r[i];
      orig[1] = patches.g[i];
      orig[2] = patches.b[i];
      float c = patches.c[i];
      if (m == 0) {
        for (int j = 0; j < 3; ++j) {
          int quantized = (int)(orig[j] * quant + 0.5f);
          rgb[j] = (int)(quantized * dequant);
        }
      } else {
        throw new IllegalStateException();
      }
      for (int j = 0; j < 3; ++j) {
        result += c * rgb[j] * (rgb[j] - 2.0f * orig[j]);
      }
    }

    return result;
  }

  static byte[] encode(float bitBudget, List<Fragment> partition, CodecParams cp) {
    int numNonLeaf = subpartition(bitBudget, partition, cp);
    //Patches patches = new Patches(numNonLeaf + 1);
    //gatherPatches(partition, numNonLeaf, patches);
    int m = cp.paletteSize;
    int n = 2 * numNonLeaf + 1;

    List<Fragment> queue = new ArrayList<>(n);
    queue.add(partition.get(0));

    XRangeEncoder dst = new XRangeEncoder();
    cp.write(dst);

    if (m != 0) throw new IllegalStateException();

    int encoded = 0;
    while (encoded < queue.size()) {
      Fragment node = queue.get(encoded++);
      boolean isLeaf = (node.ordinal >= numNonLeaf);
      node.encode(dst, cp, isLeaf, /*palette,*/ queue);
    }

    return dst.finish();
  }

  private static class TaskExecutor implements Runnable {
    private final AtomicInteger nextTask;
    private final Params params;
    private final UberCache uber;
    Variant bestVariant = null;
    float bestSqe = 1e35f;
    int bestColorCode = -1;
    List<Fragment> bestPartition = null;

    public TaskExecutor(AtomicInteger nextTask, Params params, UberCache uber) {
      this.nextTask = nextTask;
      this.params = params;
      this.uber = uber;
    }

    public void run() {
      Cache cache = new Cache(uber);
      CodecParams cp = new CodecParams(uber.width, uber.height);
      float bitBudget = uber.bitBudget(params.targetSize);
      while (true) {
        int taskId = nextTask.getAndIncrement();
        if (taskId >= params.variants.size()) return;
        Variant task = params.variants.get(taskId);
        cp.setPartitionCode(task.partitionCode);
        cp.lineLimit = task.lineLimit + 1;
        List<Fragment> partition = buildPartition(bitBudget + 8.0f, cp, cache);
        // Palette (..CodecParams.MAX_COLOR_CODE) is not supported yet.
        for (int colorCode = 0; colorCode < CodecParams.MAX_COLOR_QUANT_OPTIONS; ++colorCode) {
          if ((task.colorOptions & (1L << colorCode)) == 0) continue;
          cp.setColorCode(colorCode);
          float sqe = simulateEncode(bitBudget, partition, cp, cache);
          if (sqe < bestSqe) {
            bestSqe = sqe;
            bestVariant = task;
            bestColorCode = colorCode;
            bestPartition = partition;
          }
        }
      }
    }
  }

  static Result encode(BufferedImage src, Params params) {
    int width = src.getWidth();
    int height = src.getHeight();
    if (width < 9 || width > 2048) throw new IllegalArgumentException("width should be in the range [9..2048]");
    if (height < 9 || height > 2048) throw new IllegalArgumentException("height should be in the range [9..2048]");
    UberCache uber = new UberCache(src.getRGB(0, 0, width, height, null, 0, width), width);
    AtomicInteger nextTask = new AtomicInteger(0);

    TaskExecutor bestTaskExecutor = null;

    if ((params.numThreads == 1) || (params.variants.size() == 1)) {
      bestTaskExecutor = new TaskExecutor(nextTask, params, uber);
      // No need to use threads. Just do everything in-flow.
      bestTaskExecutor.run();
    } else {
      int numThreads = params.numThreads;
      ExecutorService executor = Executors.newFixedThreadPool(params.numThreads);
      TaskExecutor[] taskExecutors = new TaskExecutor[numThreads];
      Future<?>[] futures = new Future<?>[numThreads];
      for (int i = 0; i < numThreads; ++i) {
        taskExecutors[i] = new TaskExecutor(nextTask, params, uber);
        futures[i] = executor.submit(taskExecutors[i]);
      }
      for (int i = 0; i < numThreads; ++i) {
        try {
          futures[i].get();
          if ((i == 0) || (taskExecutors[i].bestSqe < bestTaskExecutor.bestSqe)) {
            bestTaskExecutor = taskExecutors[i];
          }
        } catch (InterruptedException e) {
          executor.shutdown();
          Thread.currentThread().interrupt();
        } catch (ExecutionException e) {
          throw new Error("ExecutionException should not occur here", e.getCause());
        }
      }
      executor.shutdown();
    }

    Variant variant = new Variant();
    variant.partitionCode = bestTaskExecutor.bestVariant.partitionCode;
    variant.lineLimit = bestTaskExecutor.bestVariant.lineLimit;
    variant.colorOptions = 1L << bestTaskExecutor.bestColorCode;
    Result result = new Result();
    result.variant = variant;
    CodecParams cp = new CodecParams(width, height);
    cp.setPartitionCode(variant.partitionCode);
    cp.lineLimit = variant.lineLimit + 1;
    cp.setColorCode(bestTaskExecutor.bestColorCode);
    result.data = encode(uber.bitBudget(params.targetSize), bestTaskExecutor.bestPartition, cp);
    result.mse = (uber.sqeBase + bestTaskExecutor.bestSqe) / ((width + 0.0f) * height);
    return result;
  }
}
