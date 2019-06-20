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
    int count = region[region.length - 1];
    int step = region.length / 3;
    int leftCount = 0;
    int leftStep = left.length / 3;
    int rightCount = 0;
    int rightStep = right.length / 3;
    if (nx == 0) {
      // nx = 0 -> ny = SinCos.SCALE -> y * ny ?? d
      for (int i = 0; i < count; i++) {
        int y = region[i];
        int x0 = region[step + i];
        int x1 = region[2 * step + i];
        if (y * ny >= d) {
          left[leftCount] = y;
          left[leftStep + leftCount] = x0;
          left[2 * leftStep + leftCount] = x1;
          leftCount++;
        } else {
          right[rightCount] = y;
          right[rightStep + rightCount] = x0;
          right[2 * rightStep + rightCount] = x1;
          rightCount++;
        }
      }
    } else {
      // nx > 0 -> x ?? (d - y * ny) / nx
      d = 2 * d + nx;
      ny = 2 * ny;
      nx = 2 * nx;
      for (int i = 0; i < count; i++) {
        int y = region[i];
        int x = (d - y * ny) / nx;
        int x0 = region[step + i];
        int x1 = region[2 * step + i];
        if (x < x1) {
          left[leftCount] = y;
          left[leftStep + leftCount] = Math.max(x, x0);
          left[2 * leftStep + leftCount] = x1;
          leftCount++;
        }
        if (x > x0) {
          right[rightCount] = y;
          right[rightStep + rightCount] = x0;
          right[2 * rightStep + rightCount] = Math.min(x, x1);
          rightCount++;
        }
      }
    }
    left[left.length - 1] = leftCount;
    right[right.length - 1] = rightCount;
  }

  static int vectorFriendlySize(int size) {
    return (size + 7) & ~7;
  }
}
