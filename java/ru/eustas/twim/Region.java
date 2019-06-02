package ru.eustas.twim;

/**
 * Region is an area of the picture.
 *
 * <p>For performance reasons, most objects are encoded as series of integers:<ul>
 *   <li>Range: v0, v1</li>
 *   <li>Scan: y, x Range</li>
 *   <li>Region: Scan * count, undefined * padding, count</li>
 * </ul></p>
 *
 * TODO(eustas): explore vectorization opportunities:
 *                - transpose Scan in Region
 */
class Region {

  // x * nx + y * ny >= d
  static void splitLine(int[] region, int angle, int d, int[] left, int[] right) {
    int nx = SinCos.SIN[angle];
    int ny = SinCos.COS[angle];
    int count3 = region[region.length - 1] * 3;
    int leftCount = 0;
    int rightCount = 0;
    if (nx == 0) {
      // nx = 0 -> ny = SinCos.SCALE -> y * ny ?? d
      for (int i = 0; i < count3; i += 3) {
        int y = region[i];
        int x0 = region[i + 1];
        int x1 = region[i + 2];
        if (y * ny >= d) {
          left[leftCount] = y;
          left[leftCount + 1] = x0;
          left[leftCount + 2] = x1;
          leftCount += 3;
        } else {
          right[rightCount] = y;
          right[rightCount + 1] = x0;
          right[rightCount + 2] = x1;
          rightCount += 3;
        }
      }
    } else {
      // nx > 0 -> x ?? (d - y * ny) / nx
      d = 2 * d + nx;
      ny = 2 * ny;
      nx = 2 * nx;
      for (int i = 0; i < count3; i += 3) {
        int y = region[i];
        int x = (d - y * ny) / nx;
        int x0 = region[i + 1];
        int x1 = region[i + 2];
        if (x < x1) {
          left[leftCount] = y;
          left[leftCount + 1] = Math.max(x, x0);
          left[leftCount + 2] = x1;
          leftCount += 3;
        }
        if (x > x0) {
          right[rightCount] = y;
          right[rightCount + 1] = x0;
          right[rightCount + 2] = Math.min(x, x1);
          rightCount += 3;
        }
      }
    }
    left[left.length - 1] = leftCount / 3;
    right[right.length - 1] = rightCount / 3;
  }
}
