import {forEachScan, int32ArraySet, last} from './Mini.js';
import * as SinCos from './SinCos.js';

/**
 * @param{!Int32Array} region
 * @param{number} angle
 * @param{number} d
 * @param{!Int32Array} left
 * @param{!Int32Array} right
 * @return {void}
 */
export let splitLine = (region, angle, d, left, right) => {
  // x * nx + y * ny >= d
  let /** @type{number} */ nx = SinCos.SIN[angle];
  let /** @type{number} */ ny = SinCos.COS[angle];
  let /** @type{number} */ leftCount = 0;
  let /** @type{number} */ rightCount = 0;
  if (nx === 0) {
    // nx = 0 -> ny = SinCos.SCALE -> y * ny ?? d
    forEachScan(region, (y, x0, x1) => {
      if (y * ny >= d) {
        int32ArraySet(left, [y, x0, x1], 3 * leftCount++);
      } else {
        int32ArraySet(right, [y, x0, x1], 3 * rightCount++);
      }
    });
  } else {
    // nx > 0 -> x ?? (d - y * ny) / nx
    d = 2 * d + nx;
    ny = 2 * ny;
    nx = 2 * nx;
    forEachScan(region, (y, x0, x1) => {
      let /** @type{number} */ x = ((d - y * ny) / nx) | 0;
      if (x < x1) {
        int32ArraySet(left, [y, x > x0 ? x : x0, x1], 3 * leftCount++);
      }
      if (x > x0) {
        int32ArraySet(right, [y, x0, x < x1 ? x : x1], 3 * rightCount++);
      }
    });
  }
  left[last(left)] = leftCount;
  right[last(right)] = rightCount;
};
