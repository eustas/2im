package ru.eustas.twim;

import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.PriorityQueue;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import static ru.eustas.twim.RangeEncoder.writeNumber;

public class Encoder {

  static class StatsCache {
    int count;
    final float[] y;
    final int[] rowOffset;
    final float[] x0;
    final float[] x1;
    final float[] x;

    int[] plus = new int[7];
    int[] minus = new int[7];

    static void sum(Cache cache, float[] regionX, int[] dst) {
      StatsCache statsCache = cache.statsCache;
      int count = statsCache.count;
      int[] rowOffset = statsCache.rowOffset;
      int[] sumR = cache.sumR;
      int[] sumG = cache.sumG;
      int[] sumB = cache.sumB;
      int[] sumR2 = cache.sumR2;
      int[] sumG2 = cache.sumG2;
      int[] sumB2 = cache.sumB2;
      int pixelCount = 0, r = 0, g = 0, b = 0, r2 = 0, g2 = 0, b2 = 0;

      for (int i = 0; i < count; i++) {
        int x = (int) regionX[i];
        int offset = rowOffset[i] + x;
        pixelCount += x;
        r += sumR[offset];
        g += sumG[offset];
        b += sumB[offset];
        r2 += sumR2[offset];
        g2 += sumG2[offset];
        b2 += sumB2[offset];
      }

      dst[0] = pixelCount;
      dst[1] = r;
      dst[2] = g;
      dst[3] = b;
      dst[4] = r2;
      dst[5] = g2;
      dst[6] = b2;
    }

    StatsCache(int height) {
      int step = Region.vectorFriendlySize(height);
      this.y = new float[step];
      this.x0 = new float[step];
      this.x1 = new float[step];
      this.x = new float[step];
      this.rowOffset = new int[step];
    }

    StatsCache(StatsCache other) {
      int step = other.x.length;
      this.y = new float[step];
      this.x0 = new float[step];
      this.x1 = new float[step];
      this.x = new float[step];
      this.rowOffset = new int[step];
    }

    void prepare(Cache cache, int[] region) {
      int count = region[region.length - 1];
      int step = region.length / 3;
      int sumStride = cache.sumStride;
      for (int i = 0; i < count; ++i) {
        int row = region[i];
        y[i] = row;
        x0[i] = region[step + i];
        x1[i] = region[2 * step + i];
        rowOffset[i] = row * sumStride;
      }
      this.count = count;
      sum(cache, x1, plus);
    }
  }

  static class Cache {
    /* Cumulative sum of previous row values. Extra column with total sum. */
    final int[] sumR;
    final int[] sumG;
    final int[] sumB;
    /* Cumulative sum of squares of previous row values. Extra column with total sum. */
    final int[] sumR2;
    final int[] sumG2;
    final int[] sumB2;
    final int sumStride;

    double[] leftScore = new double[3];
    double[] rightScore = new double[3];

    final DistanceRange distanceRange = new DistanceRange();

    final StatsCache statsCache;

    final Stats[] stats = new Stats[CodecParams.MAX_LINE_LIMIT + 6];
    {
      for (int i = 0; i < stats.length; ++i) {
        stats[i] = new Stats();
      }
    }

    Cache(int[] intRgb, int width) {
      int height = intRgb.length / width;
      if (width * height != intRgb.length) {
        throw new IllegalArgumentException("width * height != length");
      }

      int stride = width + 1;
      int[] sumR = new int[stride * height];
      int[] sumG = new int[stride * height];
      int[] sumB = new int[stride * height];
      int[] sumR2 = new int[stride * height];
      int[] sumG2 = new int[stride * height];
      int[] sumB2 = new int[stride * height];
      for (int y = 0; y < height; ++y) {
        int srcRowOffset = y * width;
        int dstRowOffset = y * stride;
        for (int x = 0; x < width; ++x) {
          int dstOffset = dstRowOffset + x;
          int rgbValue = intRgb[srcRowOffset + x];
          int r = (rgbValue >> 16) & 0xFF;
          int g = (rgbValue >> 8) & 0xFF;
          int b = rgbValue & 0xFF;
          sumR[dstOffset + 1] = sumR[dstOffset] + r;
          sumR2[dstOffset + 1] = sumR2[dstOffset] + r * r;
          sumG[dstOffset + 1] = sumG[dstOffset] + g;
          sumG2[dstOffset + 1] = sumG2[dstOffset] + g * g;
          sumB[dstOffset + 1] = sumB[dstOffset] + b;
          sumB2[dstOffset + 1] = sumB2[dstOffset] + b * b;
        }
      }
      this.sumR = sumR;
      this.sumG = sumG;
      this.sumB = sumB;
      this.sumR2 = sumR2;
      this.sumG2 = sumG2;
      this.sumB2 = sumB2;
      this.sumStride = stride;
      this.statsCache = new StatsCache(height);
    }

    Cache(Cache other) {
      this.sumR = other.sumR;
      this.sumG = other.sumG;
      this.sumB = other.sumB;
      this.sumR2 = other.sumR2;
      this.sumG2 = other.sumG2;
      this.sumB2 = other.sumB2;
      this.sumStride = other.sumStride;
      this.statsCache = new StatsCache(other.statsCache);
    }
  }

  private static class CachePool {
    Cache origin;

    ArrayList<Cache> pool = new ArrayList<>();

    CachePool(Cache origin) {
      this.origin = origin;
      pool.add(origin);
    }

    synchronized Cache take() {
      if (pool.isEmpty()) {
        return new Cache(origin);
      }
      return pool.remove(pool.size() - 1);
    }

    synchronized void release(Cache cache) {
      pool.add(cache);
    }
  }

  private static class Stats {
    final int[] rgb = new int[3];
    final int[] rgb2 = new int[3];
    int pixelCount;

    // Calculate stats from own region.
    void update(Cache cache) {
      delta(cache, cache.statsCache.x0);
    }

    void update(Stats parent, Stats sibling) {
      copy(parent);
      sub(sibling);
    }

    void copy(Stats other) {
      pixelCount = other.pixelCount;
      for (int c = 0; c < 3; ++c) {
        rgb[c] = other.rgb[c];
        rgb2[c] = other.rgb2[c];
      }
    }

    void sub(Stats other) {
      pixelCount -= other.pixelCount;
      for (int c = 0; c < 3; ++c) {
        rgb[c] -= other.rgb[c];
        rgb2[c] -= other.rgb2[c];
      }
    }

    // y * ny >= d
    static void updateGeHorizontal(Cache cache, int d) {
      int ny = SinCos.COS[0];
      float dny = d / (float) ny;
      StatsCache statsCache = cache.statsCache;
      int count = statsCache.count;
      float[] regionY = statsCache.y;
      float[] regionX0 = statsCache.x0;
      float[] regionX1 = statsCache.x1;
      float[] regionX = statsCache.x;

      for (int i = 0; i < count; i++) {
        float y = regionY[i];
        float x0 = regionX0[i];
        float x1 = regionX1[i];
        regionX[i] = ((y - dny) < 0) ? x1 : x0;
      }
    }

    // x >= (d - y * ny) / nx
    static void updateGeGeneric(Cache cache, int angle, int d) {
      int nx = SinCos.SIN[angle];
      int ny = SinCos.COS[angle];
      float dnx = (2 * d + nx) / (float) (2 * nx);
      float mnynx = -(2 * ny) / (float) (2 * nx);
      StatsCache statsCache = cache.statsCache;
      int count = statsCache.count;
      float[] regionY = statsCache.y;
      float[] regionX0 = statsCache.x0;
      float[] regionX1 = statsCache.x1;
      float[] regionX = statsCache.x;

      for (int i = 0; i < count; i++) {
        float y = regionY[i];
        float xf = Math.fma(y, mnynx, dnx);
        float x0 = regionX0[i];
        float x1 = regionX1[i];
        float m = ((xf - x0) > 0) ? xf : x0;
        regionX[i] = ((x1 - xf) > 0) ? m : x1;
      }
    }

    void delta(Cache cache, float[] regionX) {
      StatsCache statsCache = cache.statsCache;
      int[] minus = statsCache.minus;
      StatsCache.sum(cache, regionX, minus);

      int[] plus = statsCache.plus;
      this.pixelCount = plus[0] - minus[0];
      this.rgb[0] = plus[1] - minus[1];
      this.rgb[1] = plus[2] - minus[2];
      this.rgb[2] = plus[3] - minus[3];
      this.rgb2[0] = plus[4] - minus[4];
      this.rgb2[1] = plus[5] - minus[5];
      this.rgb2[2] = plus[6] - minus[6];
    }

    void updateGe(Cache cache, int angle, int d) {
      if (angle == 0) {
        updateGeHorizontal(cache, d);
      } else {
        updateGeGeneric(cache, angle, d);
      }
      delta(cache, cache.statsCache.x);
    }
  }

  private static double score(Stats parent, Stats left, Stats right, double[] leftScore, double[] rightScore) {
    if (left.pixelCount == 0 || right.pixelCount == 0) {
      return 0;
    }

    for (int c = 0; c < 3; ++c) {
      double parentAverage = (parent.rgb[c] + 0.0) / parent.pixelCount;
      double leftAverage = (left.rgb[c] + 0.0) / left.pixelCount;
      double rightAverage = (right.rgb[c] + 0.0) / right.pixelCount;
      double parentAverage2 = parentAverage * parentAverage;
      double leftAverage2 = leftAverage * leftAverage;
      double rightAverage2 = rightAverage * rightAverage;
      leftScore[c] = left.pixelCount * (parentAverage2 - leftAverage2) - 2.0 * left.rgb[c] * (parentAverage - leftAverage);
      rightScore[c] = right.pixelCount * (parentAverage2 - rightAverage2) - 2.0 * right.rgb[c] * (parentAverage - rightAverage);
    }

    double sumLeft = 0;
    double sumRight = 0;
    for (int c = 0; c < 3; ++c) {
      sumLeft += leftScore[c];
      sumRight += rightScore[c];
    }

    return sumLeft + sumRight;
  }

  static class Fragment {
    final int[] region;
    Fragment leftChild;
    Fragment rightChild;

    final Stats stats = new Stats();

    // Subdivision.
    int level;
    int bestAngleCode;
    int bestLine;
    double bestScore;
    int bestNumLines;
    double bestCost;

    private Fragment(int[] region) {
      this.region = region;
    }

    void findBestSubdivision(Cache cache, CodecParams cp) {
      DistanceRange distanceRange = cache.distanceRange;

      int[] region = this.region;
      Stats stats = this.stats;
      double[] leftScore = cache.leftScore;
      double[] rightScore = cache.rightScore;
      Stats tmpStats1 = cache.stats[cache.stats.length - 1];
      Stats tmpStats2 = cache.stats[cache.stats.length - 2];
      Stats tmpStats3 = cache.stats[cache.stats.length - 3];
      int level = cp.getLevel(region);
      int angleMax = 1 << cp.angleBits[level];
      int angleMult = (SinCos.MAX_ANGLE / angleMax);
      int bestAngleCode = 0;
      int bestLine = 0;
      double bestScore = -1.0;
      cache.statsCache.prepare(cache, region);
      stats.update(cache);

      // Find subdivision
      for (int angleCode = 0; angleCode < angleMax; ++angleCode) {
        int angle = angleCode * angleMult;
        distanceRange.update(region, angle, cp);
        int numLines = distanceRange.numLines;
        for (int line = 0; line < numLines; ++line) {
          cache.stats[line + 1].updateGe(cache, angle, distanceRange.distance(line));
        }
        cache.stats[numLines + 1].copy(stats);

        for (int line = 0; line < numLines; ++line) {
          double fullScore = 0.0;

          tmpStats3.update(cache.stats[line + 2], cache.stats[line]);
          tmpStats2.update(tmpStats3, cache.stats[line + 1]);
          tmpStats1.update(tmpStats3, tmpStats2);
          fullScore += 0.0 * score(tmpStats3, tmpStats1, tmpStats2, leftScore, rightScore);

          tmpStats1.update(stats, cache.stats[line + 1]);
          fullScore += 1.0 * score(stats, cache.stats[line + 1], tmpStats1, leftScore, rightScore);

          if (fullScore > bestScore) {
            bestAngleCode = angleCode;
            bestLine = line;
            bestScore = fullScore;
          }
        }
      }

      this.level = level;
      this.bestScore = bestScore;

      if (bestScore < 0.0) {
        this.bestCost = -1.0;
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

    void encode(RangeEncoder dst, CodecParams cp, boolean isLeaf, List<Fragment> children) {
      if (isLeaf) {
        writeNumber(dst, CodecParams.NODE_TYPE_COUNT, CodecParams.NODE_FILL);
        for (int c = 0; c < 3; ++c) {
          int q = cp.colorQuant;
          int d = 255 * stats.pixelCount;
          int v = (2 *  (q - 1) * stats.rgb[c] + d) / (2 * d);
          writeNumber(dst, q, v);
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

    void simulateEncode(CodecParams cp, boolean isLeaf, List<Fragment> children, double[] sqe) {
      if (isLeaf) {
        for (int c = 0; c < 3; ++c) {
          // sum((vi - v)^2) = sum(vi^2 - 2 * vi * v + v^2) = n * v^2 + sum(vi^2) - 2 * v * sum(vi)
          int q = cp.colorQuant;
          int d = 255 * stats.pixelCount;
          int v = (2 * (q - 1) * stats.rgb[c] + d) / (2 * d);
          int vq = CodecParams.dequantizeColor(v, q);
          sqe[c] += stats.pixelCount * vq * vq + stats.rgb2[c] - 2 * vq * stats.rgb[c];
        }
        return;
      }
      children.add(leftChild);
      children.add(rightChild);
    }
  }

  private static double bitCost(int range) {
    return Math.log(range) / Math.log(2.0);
  }

  private static int sizeCost(int value) {
    if (value < 8) throw new IllegalArgumentException("size < 8");
    value -= 8;
    int bits = 6;
    while (value > (1 << bits)) {
      value -= (1 << bits);
      bits += 3;
    }
    return bits + (bits / 3) - 1;
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
  static List<Fragment> buildPartition(int sizeLimit, CodecParams cp, Cache cache) {
    double tax = bitCost(CodecParams.NODE_TYPE_COUNT) + 3.0 * bitCost(CodecParams.makeColorQuant(0));
    double budget = sizeLimit * 8 - tax - bitCost(CodecParams.TAX);  // Minus flat image cost.

    int width = cp.width;
    int height = cp.height;
    int step = Region.vectorFriendlySize(height);
    int[] root = new int[step * 3 + 1];
    for (int y = 0; y < height; ++y) {
      root[y] = y;
      root[step + y] = 0;
      root[2 * step + y] = width;
    }
    root[root.length - 1] = height;
    Fragment rootFragment = new Fragment(root);

    List<Fragment> result = new ArrayList<>();
    PriorityQueue<Fragment> queue = new PriorityQueue<>(16, FRAGMENT_COMPARATOR);
    rootFragment.findBestSubdivision(cache, cp);
    queue.add(rootFragment);
    while (!queue.isEmpty()) {
      Fragment candidate = queue.poll();
      if (candidate.bestScore < 0.0 || candidate.bestCost < 0.0) break;
      if (tax + candidate.bestCost <= budget) {
        budget -= tax + candidate.bestCost;
        result.add(candidate);
        candidate.leftChild.findBestSubdivision(cache, cp);
        queue.add(candidate.leftChild);
        candidate.rightChild.findBestSubdivision(cache, cp);
        queue.add(candidate.rightChild);
      }
    }
    return result;
  }

  private static Set<Fragment> subpartition(int targetSize, List<Fragment> partition, CodecParams cp) {
    double tax = bitCost(CodecParams.NODE_TYPE_COUNT * cp.colorQuant * cp.colorQuant * cp.colorQuant);
    double budget = 8 * targetSize - bitCost(CodecParams.TAX) - tax;
    budget -= sizeCost(cp.width);
    budget -= sizeCost(cp.height);
    Set<Fragment> nonLeaf = new HashSet<>();
    for (Fragment node : partition) {
      if (node.bestCost < 0.0) break;
      double cost = node.bestCost + tax;
      if (budget < cost) break;
      budget -= cost;
      nonLeaf.add(node);
    }
    return nonLeaf;
  }

  private static double simulateEncode(int targetSize, List<Fragment> partition, CodecParams cp) {
    Set<Fragment> nonLeaf = subpartition(targetSize, partition, cp);
    List<Fragment> queue = new ArrayList<>(2 * nonLeaf.size() + 1);
    queue.add(partition.get(0));

    double[] sqe = new double[3];

    int encoded = 0;
    while (encoded < queue.size()) {
      Fragment node = queue.get(encoded++);
      node.simulateEncode(cp, !nonLeaf.contains(node), queue, sqe);
    }

    return sqe[0] + sqe[1] + sqe[2];
  }

  static byte[] encode(int targetSize, List<Fragment> partition, CodecParams cp) {
    Set<Fragment> nonLeaf = subpartition(targetSize, partition, cp);
    List<Fragment> queue = new ArrayList<>(2 * nonLeaf.size() + 1);
    queue.add(partition.get(0));

    RangeEncoder dst = new RangeEncoder();
    cp.write(dst);

    int encoded = 0;
    while (encoded < queue.size()) {
      Fragment node = queue.get(encoded++);
      node.encode(dst, cp, !nonLeaf.contains(node), queue);
    }

    return dst.finish();
  }

  private static class SimulationTask implements Callable<SimulationTask> {
    final int targetSize;
    final CachePool cachePool;
    final CodecParams cp;
    double bestSqe = 1e20;
    int bestColorCode = -1;

    SimulationTask(int targetSize, CachePool cachePool, CodecParams cp) {
      this.targetSize = targetSize;
      this.cachePool = cachePool;
      this.cp = cp;
    }

    @Override
    public SimulationTask call() {
      Cache cache = null;
      try {
        cache = cachePool.take();
        List<Fragment> partition = buildPartition(targetSize, cp, cache);
        for (int colorCode = 0; colorCode < CodecParams.MAX_COLOR_CODE; ++colorCode) {
          cp.setColorCode(colorCode);
          double sqe = simulateEncode(targetSize, partition, cp);
          if (sqe < bestSqe) {
            bestSqe = sqe;
            bestColorCode = colorCode;
          }
        }
        cp.setColorCode(bestColorCode);
        //System.out.println(
        //    "Done cp: [" + cp + "], g: " + goalFunction + ", error: " + Math.sqrt(bestSqe / (cp.width * cp.height)));
        return this;
      } finally {
        if (cache != null) cachePool.release(cache);
      }
    }
  }

  static byte[] encode(BufferedImage src, int targetSize) throws InterruptedException {
    int width = src.getWidth();
    int height = src.getHeight();
    Cache cache = new Cache(src.getRGB(0, 0, width, height, null, 0, width), width);
    CachePool cachePool = new CachePool(cache);
    List<SimulationTask> tasks = new ArrayList<>();
    for (int lineLimit = 0; lineLimit < CodecParams.MAX_LINE_LIMIT; ++lineLimit) {
      //if (lineLimit != 17) continue;
      for (int partitionCode = 0; partitionCode < CodecParams.MAX_PARTITION_CODE; ++partitionCode) {
        CodecParams cp = new CodecParams(width, height);
        cp.setPartitionCode(partitionCode);
        cp.lineLimit = lineLimit + 1;
        tasks.add(new SimulationTask(targetSize, cachePool, cp));
      }
    }
    int numCores = Runtime.getRuntime().availableProcessors();
    ExecutorService executor = Executors.newFixedThreadPool(numCores);
    long t0 = System.nanoTime();
    executor.invokeAll(tasks);
    executor.shutdownNow();
    SimulationTask bestResult = tasks.get(0);
    double bestSqe = 1e20;
    for (SimulationTask task : tasks) {
      if (task.bestSqe < bestSqe) {
        bestResult = task;
        bestSqe = task.bestSqe;
      }
    }
    bestResult.cp.setColorCode(bestResult.bestColorCode);
    List<Fragment> partition = buildPartition(targetSize, bestResult.cp, cache);
    byte[] data = encode(targetSize, partition, bestResult.cp);
    long t1 = System.nanoTime();
    System.out.println("time: " + (((t1 - t0) / 1000000) / 1000.0) + "s, size: " + data.length +
        ", cp: [" + bestResult.cp + "], error: " + Math.sqrt(bestSqe / (width * height)));
    return data;
  }
}
