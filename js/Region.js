import {b32, forEachScan, int32ArraySet, last} from './Mini.js';
import * as SinCos from './SinCos.js';
import {CodecParams} from './CodecParams.js';

/**
 * @constructor
 */
export function DistanceRange() {
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
 * @param{!DistanceRange} self
 * @param{!Int32Array} region
 * @param{number} angle
 * @param{!CodecParams} cp
 * @return {void}
 */
export let update = (self, region, angle, cp) => {
  // Note: SinCos.SIN is all positive -> d0 <= d1
  let /** @type{number} */ nx = SinCos.SIN[angle];
  let /** @type{number} */ ny = SinCos.COS[angle];
  let /** @type{number} */ mi = b32;
  let /** @type{number} */ ma = -b32;
  forEachScan(region, (y, x0, x1) => {
    let /** @type{number} */ d0 = ny * y + nx * x0;
    let /** @type{number} */ d1 = ny * y + nx * x1 - nx;
    mi = mi < d0 ? mi : d0;
    ma = ma > d1 ? ma : d1;
  });
  if (ma < mi) throw 4;
  self._min = mi;
  self._max = ma;
  let /** @type{number} */ lineQuant = cp.lineQuant;
  while (true) {
    self.numLines = (ma - mi) / lineQuant | 0;
    if (self.numLines > cp.lineLimit) {
      lineQuant = lineQuant + (lineQuant / 16 | 0);
    } else {
      break;
    }
  }
  self.lineQuant = lineQuant;
};

/**
 * @param{!DistanceRange} self
 * @param{number} line
 * @return {number}
 */
export let distance = (self, line) => {
  if (self.numLines > 1) {
    return self._min + (((self._max - self._min) - (self.numLines - 1) * self.lineQuant) / 2 | 0) + self.lineQuant * line;
  } else {
    return ((self._max + self._min) / 2) | 0;
  }
};

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
