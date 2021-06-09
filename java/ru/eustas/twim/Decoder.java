package ru.eustas.twim;

import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.List;

import static ru.eustas.twim.CodecParams.NODE_FILL;
import static ru.eustas.twim.XRangeDecoder.readNumber;

public class Decoder {

  private Decoder() {}

  private static int readColor(XRangeDecoder src, CodecParams cp, int[] palette) {
    if (cp.paletteSize == 0) {
      int argb = 0xFF;  // alpha = 1
      for (int c = 0; c < 3; ++c) {
        int q = cp.colorQuant;
        argb = (argb << 8) | CodecParams.dequantizeColor(readNumber(src, q), q);
      }
      return argb;
    } else {
      return palette[readNumber(src, cp.paletteSize)];
    }
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

    boolean parse(XRangeDecoder src, CodecParams cp, int[] palette, List<Fragment> children, DistanceRange distanceRange) {
      type = readNumber(src, CodecParams.NODE_TYPE_COUNT);

      int level = cp.getLevel(region);
      if (level < 0) {
        return false;  // This happpens when region is empty.
      }

      if (type == NODE_FILL) {
        color = readColor(src, cp, palette);
        return true;
      }

      // Cutting with half-planes does not increase the number of scans.
      int step = Region.vectorFriendlySize(region[region.length - 1]);
      int[] inner = new int[3 * step + 1];
      int[] outer = new int[3 * step + 1];

      switch (type) {
        case CodecParams.NODE_HALF_PLANE: {
          int angleMax = 1 << cp.angleBits[level];
          int angleMult = (SinCos.MAX_ANGLE / angleMax);
          int angleCode = readNumber(src, angleMax);
          int angle = angleCode * angleMult;
          distanceRange.update(region, angle, cp);
          int numLines = distanceRange.numLines;
          int line = readNumber(src, numLines);
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
      return true;
    }

    void render(int width, int[] rgb) {
      if (type == NODE_FILL) {
        int step = region.length / 3;
        int count = region[region.length - 1];
        int clr = color;
        for (int i = 0; i < count; i++) {
          int y = region[i];
          int x0 = region[step + i];
          int x1 = region[2 * step + i];
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
    XRangeDecoder src = new XRangeDecoder(encoded);
    CodecParams cp = CodecParams.read(src);
    int[] palette = new int[cp.paletteSize];
    int width = cp.width;
    int height = cp.height;
    for (int j = 0; j < cp.paletteSize; ++j) {
      int argb = 0xFF;  // alpha = 1
      for (int c = 0; c < 3; ++c) {
        argb = (argb << 8) | readNumber(src, 256);
      }
      palette[j] = argb;
    }

    int step = Region.vectorFriendlySize(height);
    int[] rootRegion = new int[step * 3 + 1];
    rootRegion[rootRegion.length - 1] = height;
    for (int y = 0; y < height; ++y) {
      rootRegion[y] = y;
      rootRegion[step + y] = 0;
      rootRegion[2 * step + y] = width;
    }

    Fragment root = new Fragment(rootRegion);
    List<Fragment> children = new ArrayList<>();
    children.add(root);

    DistanceRange distanceRange = new DistanceRange();
    int cursor = 0;
    while (cursor < children.size()) {
      int checkpoint = children.size();
      for (; cursor < checkpoint; ++cursor) {
        if (!children.get(cursor).parse(src, cp, palette, children, distanceRange)) {
          return null;
        }
      }
    }

    int[] rgb = new int[width * height];
    root.render(width, rgb);
    BufferedImage output = new BufferedImage(cp.width, cp.height, BufferedImage.TYPE_INT_ARGB);
    output.setRGB(0, 0, width, height, rgb, 0, width);
    return output;
  }
}
