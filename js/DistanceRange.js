import {getLineLimit, getLineQuant} from "./CodecParams.js";
import {assertFalse, b32, forEachScan} from './Mini.js';
import * as SinCos from './SinCos.js';

let /** @type{number} */ numLines = 0;
let /** @type{number} */ _min = 0;
let /** @type{number} */ _max = 0;
let /** @type{number} */ _lineQuant = 0;

/**
 * @return{number}
 */
export let getNumLines = () => numLines;

/**
 * @param{!Int32Array} region
 * @param{number} angle
 * @return {void}
 */
export let update = (region, angle) => {
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
  assertFalse(ma < mi);
  _min = mi;
  _max = ma;
  _lineQuant = getLineQuant();
  let /** @type{number} */ delta = ma - mi;
  if (delta > getLineLimit() * _lineQuant) {
    _lineQuant = (2 * delta + getLineLimit()) / (2 * getLineLimit()) | 0;
  }
  numLines = delta / _lineQuant | 0;
};

/**
 * @param{number} line
 * @return {number}
 */
export let distance = (line) => {
  if (numLines > 1) {
    return _min + (((_max - _min) - (numLines - 1) * _lineQuant) / 2 | 0) + _lineQuant * line;
  } else {
    return ((_max + _min) / 2) | 0;
  }
};
