package ru.eustas.twim;

/**
 * Region is an area of the picture.
 *
 * <p>For performance reasons, most objects are encoded as series of integers:<ul>
 *   <li>Range: v0, v1</li>
 *   <li>Scan: y, x Range</li>
 *   <li>Region: Scan * count, undefined * padding, count</li>
 *   <li>Mask: Range * height </li>
 * </ul></p>
 *
 * TODO(eustas): explore vectorization opportunities:
 *                - transpose Scan in Region
 */
class Region {
  static class DistanceRange {
    int min;
    int max;
    int numLines;
    int lineQuant;

    void update(int[] region, int angle, int lineQuant) {
      final int count3 = region[region.length - 1] * 3;
      if (count3 == 0) {
        throw new IllegalStateException("empty region");
      }

      // Note: SinCos.SIN is all positive -> d0 <= d1
      int nx = SinCos.SIN[angle];
      int ny = SinCos.COS[angle];
      int mi = Integer.MAX_VALUE;
      int ma = Integer.MIN_VALUE;
      for (int i = 0; i < count3; i += 3) {
        int y = region[i];
        int x0 = region[i + 1];
        int x1 = region[i + 2];
        int d0 = ny * y + nx * x0;
        int d1 = ny * y + nx * x1;
        mi = Math.min(mi, d0);
        ma = Math.max(ma, d1);
      }
      min = mi;
      max = ma;
      numLines = Math.min((ma - mi) / lineQuant, 1);
      this.lineQuant = lineQuant;
    }

    int distance(int line) {
      if (numLines > 1) {
        return min + ((max - min) - (numLines - 1) * lineQuant) / 2 + lineQuant * line;
      } else {
        return (max + min) / 2;
      }
    }
  }

  static void makeLineMask(int[] mask, int width, int angle, int d) {
    int height = mask.length / 2;
    int nx = SinCos.SIN[angle];
    int ny = SinCos.COS[angle];

    if (nx == 0) { // nx == 0
      for (int y = 0; y < height; ++y) {
        double dd = d - ny * y;
        if (dd <= 0) {
          mask[2 * y] = 0;
          mask[2 * y + 1] = width;
        } else {
          mask[2 * y] = -1;
          mask[2 * y + 1] = -1;
        }
      }
    } else { // nx > 0
      for (int y = 0; y < height; ++y) {
        int x = (d - ny * y) / nx;
        if (x < 0) {
          mask[2 * y] = 0;
          mask[2 * y + 1] = width;
        } else if (x >= width) {
          mask[2 * y] = -1;
          mask[2 * y + 1] = -1;
        } else {
          mask[2 * y] = x;
          mask[2 * y + 1] = width;
        }
      }
    }
  }

  static void split(int[] region, int[] mask, int[] inner, int[] outer) {
    int innerCount3 = 0;
    int outerCount3 = 0;
    int count3 = region[region.length - 1] * 3;
    for (int i = 0; i < count3; i += 3) {
      int y = region[i];
      int x0 = region[i + 1];
      int x1 = region[i + 2];
      int mx0 = mask[2 * y];
      int mx1 = mask[2 * y + 1];
      if ((mx1 <= x0) || (mx0 >= x1)) {
        // <>[O] | [O]<>
        outer[outerCount3++] = y;
        outer[outerCount3++] = x0;
        outer[outerCount3++] = x1;
      } else if ((mx0 <= x0) && (mx1 >= x1)) {
        // <[I]>
        inner[innerCount3++] = y;
        inner[innerCount3++] = x0;
        inner[innerCount3++] = x1;
      } else if ((mx0 > x0) && (mx1 < x1)) {
        // [O<I>O]
        outer[outerCount3++] = y;
        outer[outerCount3++] = x0;
        outer[outerCount3++] = mx0;
        inner[innerCount3++] = y;
        inner[innerCount3++] = mx0;
        inner[innerCount3++] = mx1;
        outer[outerCount3++] = y;
        outer[outerCount3++] = mx1;
        outer[outerCount3++] = x1;
      } else if (mx0 <= x0) {
        // <[I>O]
        inner[innerCount3++] = y;
        inner[innerCount3++] = x0;
        inner[innerCount3++] = mx1;
        outer[outerCount3++] = y;
        outer[outerCount3++] = mx1;
        outer[outerCount3++] = x1;
      } else {
        // [O<I]>
        outer[outerCount3++] = y;
        outer[outerCount3++] = x0;
        outer[outerCount3++] = mx0;
        inner[innerCount3++] = y;
        inner[innerCount3++] = mx0;
        inner[innerCount3++] = x1;
      }
    }
    inner[inner.length - 1] = innerCount3 / 3;
    outer[outer.length - 1] = outerCount3 / 3;
  }
}
