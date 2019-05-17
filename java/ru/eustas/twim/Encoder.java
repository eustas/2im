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

  private static class Fragment {
    int[] region;
    Fragment leftChild;
    Fragment rightChild;

    // Stats.
    final int[] rgb = new int[3];
    final int[] rgb2 = new int[3];
    int pixelCount;

    // Subdivision.
    int level;
    int bestAngle;
    int bestNumLines;
    int bestLine;
    double bestScore;
    double bestCost;

    static double score(int goalFunction, Fragment parent, Fragment left, Fragment right) {
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

    // Find the best subdivision.
    void expand(CodecParams cp) {
      this.level = cp.getLevel(region);
      this.leftChild = new Fragment();
      this.rightChild = new Fragment();
      this.bestScore = -1.0;
      //FindSubdivision(cp);
      //cost = BitCost(kNodeTypes);
      //cost += cp.kAngleBits[level] + BitCost(best_d_max);
    }

    // Calculate stats from own region.
    void updateStats(Cache cache) {
      int pixelCount = 0;
      int[] rgb = this.rgb;
      int[] rgb2 = this.rgb2;
      int[] sumRgb = cache.sumRgb;
      int[] sumRgb2 = cache.sumRgb2;
      int sumStride = cache.sumStride;
      int[] region = this.region;
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

    // Calculate stats from parent and sibling.
    void updateStats(Fragment parent, Fragment sibling) {
      pixelCount = parent.pixelCount - sibling.pixelCount;
      for (int c = 0; c < 3; ++c) {
        rgb[c] = parent.rgb[c] - sibling.rgb[c];
        rgb2[c] = parent.rgb2[c] - sibling.rgb2[c];
      }
    }

    void encode(RangeEncoder dst, CodecParams cp, boolean isLeaf, List<Fragment> children) {
      if (isLeaf) {
        writeNumber(dst, CodecParams.NODE_TYPE_COUNT, CodecParams.NODE_FILL);
        // TODO: implement
        //uint32_t clr[3];
        //FindColor(&region, rgb, count, clr, cp);
        //    result[0] = (cp.kQ[0] - 1.0) * rgb[0] / (255.0 * count) + 0.5;
        //    result[1] = (cp.kQ[1] - 1.0) * rgb[1] / (255.0 * count) + 0.5;
        //    result[2] = (cp.kQ[2] - 1.0) * rgb[2] / (255.0 * count) + 0.5;
        //for (int c = 0; c < 3; ++c) {
        //  bw->WriteNumber(cp.kQ[c], clr[c]);
        //}
        return;
      }
      writeNumber(dst, CodecParams.NODE_TYPE_COUNT, CodecParams.NODE_HALF_PLANE);
      int maxAngle = 1 << cp.angleBits[level];
      writeNumber(dst, maxAngle, bestAngle);
      writeNumber(dst, bestNumLines, bestLine);
      children.add(leftChild);
      children.add(rightChild);
    }
  }

  private static class FragmentComparator implements Comparator<Fragment> {
    @Override
    public int compare(Fragment o1, Fragment o2) {
      if (o1.bestScore == o2.bestScore) {
        return 0;
      }
      return o1.bestScore > o2.bestScore ? 1 : -1;
    }
  }
  private static final FragmentComparator FRAGMENT_COMPARATOR = new FragmentComparator();

  /**
   * Builds the space partition.
   *
   * Minimal color data cost is used.
   * Partition could be used to try multiple color quantizations to see, which one gives the best result.
   */
  private static List<Fragment> buildPartition(int sizeLimit, CodecParams cp, Cache cache) {
    double tax = bitCost(CodecParams.NODE_TYPE_COUNT) + bitCost(2 * 2 * 2);  // Leaf node / just prime colors.
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
    Fragment rootFragment = new Fragment();
    rootFragment.region = root;

    List<Fragment> result = new ArrayList<>();
    PriorityQueue<Fragment> queue = new PriorityQueue<>(16, FRAGMENT_COMPARATOR);
    rootFragment.expand(cp);
    queue.add(rootFragment);
    while (!queue.isEmpty()) {
      Fragment candidate = queue.poll();
      if (tax + candidate.bestCost <= budget) {
        budget -= tax + candidate.bestCost;
        result.add(candidate);
        candidate.leftChild.expand(cp);
        queue.add(candidate.leftChild);
        candidate.rightChild.expand(cp);
        queue.add(candidate.rightChild);
      }
    }
    return result;
  }

  private static double bitCost(int range) {
    return Math.log(range) / Math.log(2.0);
  }

  private static void writeNumber(RangeEncoder dst, int max, int value) {
    if (max == 1) return;
    dst.encodeRange(value, value + 1, max);
  }

  static byte[] encode(int sizeLimit, List<Fragment> partition, CodecParams cp) {
    double tax = bitCost(CodecParams.NODE_TYPE_COUNT) + bitCost(cp.colorQuant[0] * cp.colorQuant[1] * cp.colorQuant[2]);
    double budget = 8 * sizeLimit - tax - bitCost(CodecParams.MAX_CODE);

    RangeEncoder dst = new RangeEncoder();
    writeNumber(dst, CodecParams.MAX_CODE, cp.getCode());

    Set<Fragment> nonLeaf = new HashSet<>();
    for (Fragment node : partition) {
      budget -= tax;
      budget -= node.bestCost;
      if (budget <= 0.0) break;
      nonLeaf.add(node);
    }
    if (nonLeaf.isEmpty()) {
      throw new IllegalStateException("empty node tree");
    }
    List<Fragment> queue = new ArrayList<>(nonLeaf.size() + 1);
    queue.add(partition.get(0));

    int encoded = 0;
    while (encoded < queue.size()) {
      Fragment node = queue.get(encoded++);
      boolean isLeaf = !nonLeaf.contains(node);
      node.encode(dst, cp, isLeaf, queue);
    }

    return dst.finish();
  }

  static byte[] encode(BufferedImage src) {
    int width = 64;
    int height = 64;
    if (src.getWidth() != width || src.getHeight() != height) {
      throw new IllegalArgumentException("invalid image size");
    }
    CodecParams cp = new CodecParams(width, height);
    return null;
  }
}
