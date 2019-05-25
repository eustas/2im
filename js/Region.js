import {b32, last} from './Mini.js';
import * as SinCos from './SinCos.js';

export class DistanceRange {
  constructor() {
    /** @type{number} */
    this._min = 0;
    /** @type{number} */
    this._max = 0;
    /** @type{number} */
    this.numLines = 0;
    /** @type{number} */
    this.lineQuant = 0;
  }

  /**
   * @param{!Int32Array} region
   * @param{number} angle
   * @param{number} lineQuant
   * @return {void}
   */
  update(region, angle, lineQuant) {
    let /** @type{number} */ count3 = region[last(region)] * 3;
    if (count3 === 0) {
      throw 4;
    }

    // Note: SinCos.SIN is all positive -> d0 <= d1
    let /** @type{number} */ nx = SinCos.SIN[angle];
    let /** @type{number} */ ny = SinCos.COS[angle];
    let /** @type{number} */ mi = b32;
    let /** @type{number} */ ma = -b32;
    for (let /** @type{number} */ i = 0; i < count3; i += 3) {
      let /** @type{number} */ y = region[i];
      let /** @type{number} */ x0 = region[i + 1];
      let /** @type{number} */ x1 = region[i + 2] - 1;
      let /** @type{number} */ d0 = ny * y + nx * x0;
      let /** @type{number} */ d1 = ny * y + nx * x1;
      mi = mi < d0 ? mi : d0;
      ma = ma > d1 ? ma : d1;
    }
    this._min = mi;
    this._max = ma;
    this.numLines = ((ma - mi) / lineQuant) | 0;
    this.lineQuant = lineQuant;
  }

  /**
   * @param{number} line
   * @return {number}
   */
  distance(line) {
    if (this.numLines > 1) {
      return this._min + (((this._max - this._min) - (this.numLines - 1) * this.lineQuant) / 2 | 0) + this.lineQuant * line;
    } else {
      return ((this._max + this._min) / 2) | 0;
    }
  }
}

/**
 * @param{!Int32Array} region
 * @param{number} angle
 * @param{number} d
 * @param{!Int32Array} left
 * @param{!Int32Array} right
 * @return {void}
 */
export function splitLine(region, angle, d, left, right) {
  // x * nx + y * ny >= d
  let /** @type{number} */ nx = SinCos.SIN[angle];
  let /** @type{number} */ ny = SinCos.COS[angle];
  let /** @type{number} */ count3 = region[last(region)] * 3;
  let /** @type{number} */ leftCount = 0;
  let /** @type{number} */ rightCount = 0;
  if (nx === 0) {
    // nx = 0 -> ny = SinCos.SCALE -> y * ny ?? d
    for (let /** @type{number} */ i = 0; i < count3; i += 3) {
      let /** @type{number} */ y = region[i];
      let /** @type{number} */ x0 = region[i + 1];
      let /** @type{number} */ x1 = region[i + 2];
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
    for (let /** @type{number} */ i = 0; i < count3; i += 3) {
      let /** @type{number} */ y = region[i];
      let /** @type{number} */ x = ((d - y * ny) / nx) | 0;
      let /** @type{number} */ x0 = region[i + 1];
      let /** @type{number} */ x1 = region[i + 2];
      if (x < x1) {
        left[leftCount] = y;
        left[leftCount + 1] = x > x0 ? x : x0;
        left[leftCount + 2] = x1;
        leftCount += 3;
      }
      if (x > x0) {
        right[rightCount] = y;
        right[rightCount + 1] = x0;
        right[rightCount + 2] = x < x1 ? x : x1;
        rightCount += 3;
      }
    }
  }
  left[last(left)] = leftCount / 3;
  right[last(right)] = rightCount / 3;
}
