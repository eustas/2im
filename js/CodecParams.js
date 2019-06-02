import {assertFalse, forEachScan, math, newInt32Array} from './Mini.js';
import * as SinCos from './SinCos.js';

let /** @type{number} */ MAX_LEVEL = 7;

let /** @type{number} */ MAX_F1 = 4;
let /** @type{number} */ MAX_F2 = 5;
let /** @type{number} */ MAX_F3 = 5;
let /** @type{number} */ MAX_F4 = 5;

let /** @type{number} */ MAX_PARTITION_CODE = MAX_F1 * MAX_F2 * MAX_F3 * MAX_F4;
let /** @type{number} */ MAX_COLOR_CODE = 17;

/**
 * @param{number} code
 * @return {number}
 */
let makeColorQuant = (code) => 1 + ((4 + (code & 3)) << (code >> 2));

let /** @type{!Int32Array} */ SCALE_STEP = newInt32Array([1000, 631, 399, 252, 159, 100]);
let /** @type{number} */ SCALE_STEP_FACTOR = 40;
let /** @type{number} */ BASE_SCALE_FACTOR = 36;

/** @type{number} */
let _width;
/** @type{number} */
let _height;
/** @type{number} */
let _colorQuant;
/** @type{number} */
export let lineLimit = 25;
/** @const @type{!Int32Array} */
let _levelScale = newInt32Array(MAX_LEVEL);
/** @const @type{!Int32Array} */
export let angleBits = newInt32Array(MAX_LEVEL);

/**
 * @return{number}
 */
export let getColorQuant = () => _colorQuant;

/**
 * @param{number} width
 * @param{number} height
 */
export let init = (width, height) => {
  _width = width;
  _height = height;
};

/**
 * @return {number}
 */
export let getLineQuant = () => SinCos.SCALE;

/**
 * @param{number} code
 * @return{void}
 */
export let setCode = (code) => {
  let /** @type{number} */ partitionCode = code % MAX_PARTITION_CODE;
  code = (code / MAX_PARTITION_CODE) | 0;
  let /** @type{number} */ colorCode = code % MAX_COLOR_CODE;

  let /** @type{number} */ f1 = partitionCode % MAX_F1;
  partitionCode = (partitionCode / MAX_F1) | 0;
  let /** @type{number} */ f2 = 2 + partitionCode % MAX_F2;
  partitionCode = (partitionCode / MAX_F2) | 0;
  let /** @type{number} */ f3 = partitionCode % MAX_F3;
  partitionCode = (partitionCode / MAX_F3) | 0;
  let /** @type{number} */ f4 = partitionCode % MAX_F4;

  let /** @type{number} */ scale = (_width * _width + _height * _height) * f2 * f2;
  for (let /** @type{number} */ i = 0; i < MAX_LEVEL; ++i) {
    _levelScale[i] = scale / BASE_SCALE_FACTOR;
    scale = ((scale * SCALE_STEP_FACTOR) / SCALE_STEP[f3]) | 0;
  }

  let /** @type{number} */ bits = 9 - f1;
  for (let /** @type{number} */ i = 0; i < MAX_LEVEL; ++i) {
    angleBits[i] = math.max(bits - i - (((i * f4) / 2) | 0), 0);
  }
  _colorQuant = makeColorQuant(colorCode);
};

/**
 * @param{!Int32Array} region
 * @return{number}
 */
export let getLevel = (region) => {
  let /** @type{number} */ min_y = _height + 1;
  let /** @type{number} */ max_y = -1;
  let /** @type{number} */ min_x = _width + 1;
  let /** @type{number} */ max_x = -1;
  forEachScan(region, (y, x0, x1) => {
    min_y = min_y < y ? min_y : y;
    max_y = max_y > y ? max_y : y;
    min_x = min_x < x0 ? min_x : x0;
    max_x = max_x > x1 ? max_x : x1;
  });
  assertFalse(max_y < 0);

  let /** @type{number} */ dx = max_x - min_x;
  let /** @type{number} */ dy = max_y + 1 - min_y;
  let /** @type{number} */ d = dx * dx + dy * dy;
  let /** @type{number} */ i = 0;
  for (; i < MAX_LEVEL - 1; ++i) {
    if (d >= _levelScale[i]) {
      break;
    }
  }
  return i;
};

/**
 * @param{number} v
 * @param{number} q
 * @return{number}
 */
export let dequantizeColor = (v, q) => ((255 * v + q - 2) / (q - 1)) | 0;

/** @const @type{number} */
export let MAX_CODE = MAX_PARTITION_CODE * MAX_COLOR_CODE;

/** @const @type{number} */
export let NODE_FILL = 0;
/** @const @type{number} */
export let NODE_HALF_PLANE = 1;
/** @const @type{number} */
export let NODE_TYPE_COUNT = 2;
