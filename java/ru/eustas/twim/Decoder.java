package ru.eustas.twim;

import java.awt.image.BufferedImage;

import static ru.eustas.twim.CodecParams.NODE_FILL;

public class Decoder {

  private static int readNumber(RangeDecoder src, int max) {
    int result = src.currentCount(max);
    src.removeRange(result, result + 1);
    return result;
  }

  private static int readColor(RangeDecoder src, CodecParams cp) {
    int argb = 0xFF;  // alpha = 1
    for (int c = 0; c < 3; ++c) {
      argb = (argb << 8) | cp.colorDequantTable[c][readNumber(src, cp.colorQuant[c])];
    }
    return argb;
  }

  private static class Fragment {
    private int type = NODE_FILL;
    private int color;
    private int[] region;
    private Fragment leftChild;
    private Fragment rightChild;

    Fragment(int[] region) {
      this.region = region;
    }

    void parse(RangeDecoder src, CodecParams cp, int[] mask) {
      type = readNumber(src, CodecParams.NODE_TYPE_COUNT);

      if (type == NODE_FILL) {
        color = readColor(src, cp);
        return;
      }

      int level = cp.getLevel(region);

      // TODO: remove this
      if (level < 0) {
        type = NODE_FILL;
        return;
      }
      switch (type) {
        case CodecParams.NODE_HALF_PLANE: {
          int angleMax = 1 << cp.angleBits[level];
          int angle = readNumber(src, angleMax) * (SinCos.MAX_ANGLE / angleMax);
          Region.DistanceRange distanceRange = new Region.DistanceRange();
          distanceRange.update(region, angle);
          int min = distanceRange.min;
          int max = distanceRange.max;
          int delta = max - min;
          int lineQuant = cp.lineQuant;
          int numLines = delta / lineQuant;
          int distance;
          if (numLines > 1) {
            distance = min + (delta - (numLines - 1) * lineQuant) / 2 + lineQuant * readNumber(src, numLines);
          } else {
            distance = (max + min) / 2;
          }
          Region.makeLineMask(mask, cp.width, angle, distance);
          break;
        }

        case NODE_FILL:
        default:
          throw new IllegalStateException("unreachable");
      }

      // Cutting with half-planes does not increase the number of scans.
      // TODO: region will remain unused; split to region and donate it to child?
      int lastCount3 = region[region.length - 1] * 3;
      int[] inner = new int[lastCount3 + 1];
      int[] outer = new int[lastCount3 + 1];
      Region.split(region, mask, inner, outer);
      leftChild = new Fragment(inner);
      rightChild = new Fragment(outer);

      // TODO: perhaps reorganize to layered parsing.
      leftChild.parse(src, cp, mask);
      rightChild.parse(src, cp, mask);
    }

    void render(int width, int[] rgb) {
      if (type == NODE_FILL) {
        int count3 = region[region.length - 1];
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

  static BufferedImage decode(byte[] encoded) {
    RangeDecoder src = new RangeDecoder(encoded);
    int width = 64;
    int height = 64;
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
    int[] mask = new int[height * 2];
    root.parse(src, cp, mask);

    int[] rgb = new int[width * height];
    root.render(width, rgb);
    BufferedImage output = new BufferedImage(cp.width, cp.height, BufferedImage.TYPE_INT_ARGB);
    output.setRGB(0, 0, width, height, rgb, 0, width);
    return  output;
  }
}
