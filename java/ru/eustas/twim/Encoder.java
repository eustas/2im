package ru.eustas.twim;

import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.PriorityQueue;
import java.util.Set;

public class Encoder {

  private static final double[] LUM = {0.299, 0.587, 0.114};

  static class Cache {
    /* Cumulative sum of previous row values. [R, G, B] triplets. Extra column with total sum. */
    int[] sumRgb;
    /* Cumulative sum of squares of previous row values. [R, G, B] triplets. Extra column with total sum. */
    int[] sumRgb2;
    int sumStride;

    Region.DistanceRange distanceRange = new Region.DistanceRange();

    Cache(int[] intRgb, int width) {
      int height = intRgb.length / width;
      if (width * height != intRgb.length) {
        throw new IllegalArgumentException("width * height != length");
      }

      int stride = (width + 1) * 3;
      int[] sum = new int[3];
      int[] sum2 = new int[3];
      int[] sumRgb = new int[stride * height];
      int[] sumRgb2 = new int[stride * height];
      for (int y = 0; y < height; ++y) {
        for (int c = 0; c < 3; ++c) {
          sum[c] = 0;
          sum2[c] = 0;
        }
        int srcRowOffset = y * width;
        int dstRowOffset = y * stride;
        for (int x = 0; x < width; ++x) {
          int dstOffset = dstRowOffset + 3 * x + 3;  // extra +1 for sum delay
          int rgbValue = intRgb[srcRowOffset + x];
          for (int c = 2; c >= 0; --c) {
            int value = rgbValue & 0xFF;
            rgbValue >>>= 8;
            sum[c] += value;
            sum2[c] += value * value;
            sumRgb[dstOffset + c] = sum[c];
            sumRgb2[dstOffset + c] = sum2[c];
          }
        }
      }
      this.sumRgb = sumRgb;
      this.sumRgb2 = sumRgb2;
      this.sumStride = stride;
    }
  }

  private static class Stats {
    final int[] rgb = new int[3];
    final int[] rgb2 = new int[3];
    int pixelCount;

    // Calculate stats from own region.
    void update(Cache cache, int[] region) {
      int pixelCount = 0;
      int[] rgb = this.rgb;
      int[] rgb2 = this.rgb2;
      int[] sumRgb = cache.sumRgb;
      int[] sumRgb2 = cache.sumRgb2;
      int sumStride = cache.sumStride;
      int count3 = region[region.length - 1] * 3;
      for (int c = 0; c < 3; ++c) {
        rgb[c] = 0;
        rgb2[c] = 0;
      }
      for (int i = 0; i < count3; i += 3) {
        int y = region[i];
        int x0 = region[i + 1];
        int x1 = region[i + 2];
        int ox0 = y * sumStride + 3 * x0;
        int ox1 = y * sumStride + 3 * x1;
        pixelCount += x1 - x0;
        for (int c = 0; c < 3; ++c) {
          rgb[c] += sumRgb[ox1 + c] - sumRgb[ox0 + c];
          rgb2[c] += sumRgb2[ox1 + c] - sumRgb2[ox0 + c];
        }
      }
      this.pixelCount = pixelCount;
    }

    void update(Stats parent, Stats sibling) {
      pixelCount = parent.pixelCount - sibling.pixelCount;
      for (int c = 0; c < 3; ++c) {
        rgb[c] = parent.rgb[c] - sibling.rgb[c];
        rgb2[c] = parent.rgb2[c] - sibling.rgb2[c];
      }
    }

    // x * nx + y * ny >= d
    void updateGe(Cache cache, int[] region, int angle, int d) {
      int pixelCount = 0;
      int[] rgb = this.rgb;
      int[] rgb2 = this.rgb2;
      int[] sumRgb = cache.sumRgb;
      int[] sumRgb2 = cache.sumRgb2;
      int sumStride = cache.sumStride;
      int count3 = region[region.length - 1] * 3;
      for (int c = 0; c < 3; ++c) {
        rgb[c] = 0;
        rgb2[c] = 0;
      }

      int nx = SinCos.SIN[angle];
      int ny = SinCos.COS[angle];
      int j = 0;
      if (nx == 0) {
        // nx = 0 -> ny = SinCos.SCALE -> y * ny >= d
        for (int i = 0; i < count3; i += 3) {
          int y = region[i];
          if (y * ny >= d) {
            int x0 = region[i + 1];
            int x1 = region[i + 2];
            pixelCount += x1 - x0;
            int ox0 = y * sumStride + 3 * x0;
            int ox1 = y * sumStride + 3 * x1;
            for (int c = 0; c < 3; ++c) {
              rgb[c] += sumRgb[ox1 + c] - sumRgb[ox0 + c];
              rgb2[c] += sumRgb2[ox1 + c] - sumRgb2[ox0 + c];
            }
          }
        }
      } else {
        // nx > 0 -> x >= (d - y * ny) / nx
        d = 2 * d + nx;
        nx = 2 * nx;
        ny = 2 * ny;
        for (int i = 0; i < count3; i += 3) {
          int y = region[i];
          int x = (d - y * ny) / nx;
          int x0 = region[i + 1];
          int x1 = region[i + 2];
          if (x < x1) {
            x0 = Math.max(x, x0);
            pixelCount += x1 - x0;
            int ox0 = y * sumStride + 3 * x0;
            int ox1 = y * sumStride + 3 * x1;
            for (int c = 0; c < 3; ++c) {
              rgb[c] += sumRgb[ox1 + c] - sumRgb[ox0 + c];
              rgb2[c] += sumRgb2[ox1 + c] - sumRgb2[ox0 + c];
            }
          }
        }
      }
      this.pixelCount = pixelCount;
    }
  }

  private static double score(int goalFunction, Stats parent, Stats left, Stats right) {
    if (left.pixelCount == 0 || right.pixelCount == 0) {
      return 0;
    }

    double[] leftScore = new double[3];
    double[] rightScore = new double[3];
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

    double lumLeft = 0;
    double lumRight = 0;
    double sumLeft = 0;
    double sumRight = 0;
    for (int c = 0; c < 3; ++c) {
      lumLeft += LUM[c] * Math.sqrt(leftScore[c]);
      lumRight += LUM[c] * Math.sqrt(rightScore[c]);
      sumLeft += leftScore[c];
      sumRight += rightScore[c];
    }

    // Bad score functions:
    // 1) delta ^ 2 * count
    // 2) (incorrect?) solution of MSE optimization equation
    switch (goalFunction) {
      case 0: return Math.max(lumLeft, lumRight);
      case 1: return Math.sqrt(lumLeft * lumRight);
      case 2: return lumLeft + lumRight;
      case 3: return sumLeft + sumRight;
      default: throw new RuntimeException("unknown goal function");
    }
  }

  private static class Fragment {
    final int[] region;
    Fragment leftChild;
    Fragment rightChild;

    Stats stats = new Stats();

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

    void findBestSubdivision(Cache cache, int goalFunction, CodecParams cp) {
      Region.DistanceRange distanceRange = cache.distanceRange;
      // TODO(eustas): move to cache?
      Stats left = new Stats();
      Stats right = new Stats();

      int[] region = this.region;
      Stats stats = this.stats;
      int lineQuant = cp.lineQuant;
      int level = cp.getLevel(region);
      int angleMax = 1 << cp.angleBits[level];
      int angleMult = (SinCos.MAX_ANGLE / angleMax);
      int bestAngleCode = 0;
      int bestLine = 0;
      double bestScore = -1.0;

      // Find subdivision
      for (int angleCode = 0; angleCode < angleMax; ++angleCode) {
        int angle = angleCode * angleMult;
        distanceRange.update(region, angle, lineQuant);
        int numLines = distanceRange.numLines;
        for (int line = 0; line < numLines; ++line) {
          int d = distanceRange.distance(line);
          double fullScore = 0.0;
          // TODO: stripe delta = 1, when possible
          left.updateGe(cache, region, angle, d);
          right.update(stats, left);
          fullScore += score(goalFunction, stats, left, right);
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
        distanceRange.update(region, bestAngleCode * angleMult, lineQuant);
        int[] leftRegion = new int[region[region.length - 1] * 3 + 1];
        int[] rightRegion = new int[region[region.length - 1] * 3 + 1];
        Region.splitLine(region, bestAngleCode * angleMult, distanceRange.distance(bestLine), leftRegion, rightRegion);
        this.leftChild = new Fragment(leftRegion);
        this.leftChild.stats.update(cache, leftRegion);
        this.rightChild = new Fragment(rightRegion);
        this.rightChild.stats.update(stats, this.leftChild.stats);

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
          int q = cp.colorQuant[c];
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
          int q = cp.colorQuant[c];
          int d = 255 * stats.pixelCount;
          int v = (2 *  (q - 1) * stats.rgb[c] + d) / (2 * d);
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

  private static void writeNumber(RangeEncoder dst, int max, int value) {
    if (max == 1) return;
    dst.encodeRange(value, value + 1, max);
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
  private static List<Fragment> buildPartition(int sizeLimit, int goalFunction, CodecParams cp, Cache cache) {
    double tax = bitCost(CodecParams.NODE_TYPE_COUNT) + 3.0 * bitCost(CodecParams.COLOR_QUANT[0]);
    double budget = sizeLimit * 8 - tax - bitCost(CodecParams.MAX_CODE);  // Minus flat image cost.

    int width = cp.width;
    int height = cp.height;
    int[] root = new int[height * 3 + 1];
    for (int y = 0; y < height; ++y) {
      root[3 * y] = y;
      root[3 * y + 1] = 0;
      root[3 * y + 2] = width;
    }
    root[root.length - 1] = height;
    Fragment rootFragment = new Fragment(root);
    rootFragment.stats.update(cache, root);

    List<Fragment> result = new ArrayList<>();
    PriorityQueue<Fragment> queue = new PriorityQueue<>(16, FRAGMENT_COMPARATOR);
    rootFragment.findBestSubdivision(cache, goalFunction, cp);
    queue.add(rootFragment);
    while (!queue.isEmpty()) {
      Fragment candidate = queue.poll();
      if (candidate.bestScore < 0.0 || candidate.bestCost < 0.0) break;
      if (tax + candidate.bestCost <= budget) {
        budget -= tax + candidate.bestCost;
        result.add(candidate);
        candidate.leftChild.findBestSubdivision(cache, goalFunction, cp);
        queue.add(candidate.leftChild);
        candidate.rightChild.findBestSubdivision(cache, goalFunction, cp);
        queue.add(candidate.rightChild);
      }
    }
    return result;
  }

  static double simulateEncode(int sizeLimit, List<Fragment> partition, CodecParams cp) {
    double tax = bitCost(CodecParams.NODE_TYPE_COUNT * cp.colorQuant[0] * cp.colorQuant[1] * cp.colorQuant[2]);
    double budget = 8 * sizeLimit - bitCost(CodecParams.MAX_CODE) - tax;

    Set<Fragment> nonLeaf = new HashSet<>();
    for (Fragment node : partition) {
      if (node.bestCost < 0.0) break;
      double cost = node.bestCost + tax;
      if (budget < cost) break;
      budget -= cost;
      nonLeaf.add(node);
    }
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

  private static byte[] encode(int sizeLimit, List<Fragment> partition, CodecParams cp) {
    if (partition.isEmpty()) {
      throw new IllegalStateException("empty tree");
    }

    double tax = bitCost(CodecParams.NODE_TYPE_COUNT * cp.colorQuant[0] * cp.colorQuant[1] * cp.colorQuant[2]);
    double budget = 8 * sizeLimit - bitCost(CodecParams.MAX_CODE) - tax;

    RangeEncoder dst = new RangeEncoder();
    writeNumber(dst, CodecParams.MAX_CODE, cp.getCode());

    Set<Fragment> nonLeaf = new HashSet<>();
    for (Fragment node : partition) {
      if (node.bestCost < 0.0) break;
      double cost = node.bestCost + tax;
      if (budget < cost) break;
      budget -= cost;
      nonLeaf.add(node);
    }
    List<Fragment> queue = new ArrayList<>(2 * nonLeaf.size() + 1);
    queue.add(partition.get(0));

    int encoded = 0;
    while (encoded < queue.size()) {
      Fragment node = queue.get(encoded++);
      node.encode(dst, cp, !nonLeaf.contains(node), queue);
    }

    return dst.finish();
  }

  static byte[] encode(BufferedImage src, int targetSize) {
    int width = 64;
    int height = 64;
    if (src.getWidth() != width || src.getHeight() != height) {
      throw new IllegalArgumentException("invalid image size");
    }
    Cache cache = new Cache(src.getRGB(0, 0, width, height, null, 0, width), width);
    CodecParams cp = new CodecParams(width, height);
    List<Fragment> partition = null;
    double bestSqe = 1e20;
    int bestPartitionCode = -1;
    int bestColorCode = -1;
    int bestGoalFunction = -1;
    for (int partitionCode = 0; partitionCode < CodecParams.MAX_PARTITION_CODE; ++partitionCode) {
    cp.setPartitionCode(partitionCode);
    for (int goalFunction = 0; goalFunction < 4; ++goalFunction) {
      long t0 = System.nanoTime();
      partition = buildPartition(targetSize, goalFunction, cp, cache);
      for (int colorCode = 0; colorCode < CodecParams.MAX_COLOR_CODE; ++colorCode) {
        cp.setColorCode(colorCode);
        double sqe = simulateEncode(targetSize, partition, cp);
        if (sqe < bestSqe) {
          System.out.println(">>> g=" + goalFunction + " c=" + colorCode + " p=" + partitionCode +  " | " + sqe);
          bestSqe = sqe;
          bestColorCode = colorCode;
          bestGoalFunction = goalFunction;
          bestPartitionCode = partitionCode;
        }
      }
      long t1 = System.nanoTime();
      //System.out.println(((t1 - t0) / 1000 / 1000.0) + "ms g=" + goalFunction + " p=" + partitionCode);
    }
    }
    cp.setColorCode(bestColorCode);
    cp.setPartitionCode(bestPartitionCode);
    partition = buildPartition(targetSize, bestGoalFunction, cp, cache);
    return encode(targetSize, partition, cp);
  }
}
