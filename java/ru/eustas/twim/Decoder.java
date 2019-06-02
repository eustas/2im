package ru.eustas.twim;

import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.List;

import static ru.eustas.twim.CodecParams.NODE_FILL;

public class Decoder {
  private static int readNumber(RangeDecoder src, int max) {
    if (max < 2) return 0;
    int result = src.currentCount(max);
    src.removeRange(result, result + 1);
    return result;
  }

  private static int readColor(RangeDecoder src, CodecParams cp) {
    int argb = 0xFF;  // alpha = 1
    for (int c = 0; c < 3; ++c) {
      int q = cp.colorQuant;
      argb = (argb << 8) | CodecParams.dequantizeColor(readNumber(src, q), q);
    }
    return argb;
  }

  private static class Fragment {
    private int type = NODE_FILL;
    private int color;
    private final int[] region;
    private Fragment leftChild;
    private Fragment rightChild;

    Fragment(int[] region) {
      this.region = region;
    }

    void parse(RangeDecoder src, CodecParams cp, List<Fragment> children, DistanceRange distanceRange) {
      type = readNumber(src, CodecParams.NODE_TYPE_COUNT);

      if (type == NODE_FILL) {
        color = readColor(src, cp);
        return;
      }

      int level = cp.getLevel(region);
      if (level < 0) {
        throw new IllegalStateException("corrupted input");
      }

      // Cutting with half-planes does not increase the number of scans.
      // TODO: region will remain unused; split to region and donate it to child?
      int lastCount3 = region[region.length - 1] * 3;
      int[] inner = new int[lastCount3 + 1];
      int[] outer = new int[lastCount3 + 1];

      switch (type) {
        case CodecParams.NODE_HALF_PLANE: {
          int angleMax = 1 << cp.angleBits[level];
          int angleMult = (SinCos.MAX_ANGLE / angleMax);
          int angleCode = readNumber(src, angleMax);
          int angle = angleCode * angleMult;
          distanceRange.update(region, angle, cp);
          int line = readNumber(src, distanceRange.numLines);
          Region.splitLine(region, angle, distanceRange.distance(line), inner, outer);
          break;
        }

        case NODE_FILL:
        default:
          throw new IllegalStateException("unreachable");
      }

      leftChild = new Fragment(inner);
      children.add(leftChild);
      rightChild = new Fragment(outer);
      children.add(rightChild);
    }

    void render(int width, int[] rgb) {
      if (type == NODE_FILL) {
        int count3 = region[region.length - 1] * 3;
        int clr = color;
        for (int i = 0; i < count3; i += 3) {
          int y = region[i];
          int x0 = region[i + 1];
          int x1 = region[i + 2];
          for (int x = x0; x < x1; ++x) {
            rgb[y * width + x] = clr;
          }
        }
        return;
      }
      leftChild.render(width, rgb);
      rightChild.render(width, rgb);
    }
  }

  static BufferedImage decode(byte[] encoded, int width, int height) {
    RangeDecoder src = new RangeDecoder(encoded);
    CodecParams cp = new CodecParams(width, height);
    cp.setCode(readNumber(src, CodecParams.MAX_CODE));

    int[] rootRegion = new int[height * 3 + 1];
    rootRegion[rootRegion.length - 1] = height;
    for (int y = 0; y < height; ++y) {
      rootRegion[y * 3] = y;
      rootRegion[y * 3 + 1] = 0;
      rootRegion[y * 3 + 2] = width;
    }

    Fragment root = new Fragment(rootRegion);
    List<Fragment> children = new ArrayList<>();
    children.add(root);

    DistanceRange distanceRange = new DistanceRange();
    int cursor = 0;
    while (cursor < children.size()) {
      int checkpoint = children.size();
      for (; cursor < checkpoint; ++cursor) {
        children.get(cursor).parse(src, cp, children, distanceRange);
      }
    }

    int[] rgb = new int[width * height];
    root.render(width, rgb);
    BufferedImage output = new BufferedImage(cp.width, cp.height, BufferedImage.TYPE_INT_ARGB);
    output.setRGB(0, 0, width, height, rgb, 0, width);
    return output;
  }
}
