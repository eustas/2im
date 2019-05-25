import {last, newInt32Array} from './Mini.js';
import * as SinCos from './SinCos.js';

const /** @type{number} */ MAX_LEVEL = 7;

const /** @type{number} */ MAX_F1 = 4;
const /** @type{number} */ MAX_F2 = 5;
const /** @type{number} */ MAX_F3 = 5;
const /** @type{number} */ MAX_F4 = 5;

const /** @type{number} */ MAX_PARTITION_CODE = MAX_F1 * MAX_F2 * MAX_F3 * MAX_F4;
const /** @type{number} */ MAX_COLOR_CODE = 30;

const /** @type{!Int32Array} */ COLOR_QUANT = newInt32Array([10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54]);

const /** @type{!Int32Array} */ SCALE_STEP = newInt32Array([1000, 631, 399, 252, 159, 100]);
const /** @type{number} */ SCALE_STEP_FACTOR = 40;
const /** @type{!Int32Array} */ BASE_SCALE = newInt32Array([4, 9, 16, 25, 36]);
const /** @type{number} */ BASE_SCALE_FACTOR = 36;

export class CodecParams {
  /**
   * @param{number} width
   * @param{number} height
   */
  constructor(width, height) {
    /** @const @type{number} */
    this._width = width;
    /** @const @type{number} */
    this._height = height;
    /** @const @type{number} */
    this.lineQuant = SinCos.SCALE;
    /** @const @type{!Int32Array} */
    this.levelScale = newInt32Array(MAX_LEVEL);
    /** @const @type{!Int32Array} */
    this.angleBits = newInt32Array(MAX_LEVEL);
    /** @const @type{!Int32Array} */
    this.colorQuant = newInt32Array(3);
  }

  /**
   * @param{number} code
   * @return{void}
   */
  setCode(code) {
    let /** @type{number} */ partitionCode = code % MAX_PARTITION_CODE;
    code = (code / MAX_PARTITION_CODE) | 0;
    let /** @type{number} */ colorCode = code % MAX_COLOR_CODE;

    let /** @type{number} */ f1 = partitionCode % MAX_F1;
    partitionCode = (partitionCode / MAX_F1) | 0;
    let /** @type{number} */ f2 = partitionCode % MAX_F2;
    partitionCode = (partitionCode / MAX_F2) | 0;
    let /** @type{number} */ f3 = partitionCode % MAX_F3;
    partitionCode = (partitionCode / MAX_F3) | 0;
    let /** @type{number} */ f4 = partitionCode % MAX_F4;

    let /** @type{number} */ scale = (this._width * this._width + this._height * this._height) * BASE_SCALE[f2];
    for (let /** @type{number} */ i = 0; i < MAX_LEVEL; ++i) {
      this.levelScale[i] = scale / BASE_SCALE_FACTOR;
      scale = ((scale * SCALE_STEP_FACTOR) / SCALE_STEP[f3]) | 0;
    }

    let /** @type{number} */ bits = 9 - f1;
    for (let /** @type{number} */ i = 0; i < MAX_LEVEL; ++i) {
      this.angleBits[i] = Math.max(bits - i - (((i * f4) / 2) | 0), 0);
    }

    for (let /** @type{number} */ c = 0; c < 3; ++c) {
      this.colorQuant[c] = COLOR_QUANT[colorCode];
    }
  }

  /**
   * @param{!Int32Array} region
   * @return{number}
   */
  getLevel(region) {
    const /** @type{number} */ count3 = region[last(region)] * 3;
    if (count3 === 0) {
      return -1;
    }

    let /** @type{number} */ min_y = this._height + 1;
    let /** @type{number} */ max_y = -1;
    let /** @type{number} */ min_x = this._width + 1;
    let /** @type{number} */ max_x = -1;
    for (let /** @type{number} */ i = 0; i < count3; i += 3) {
      let /** @type{number} */ y = region[i];
      let /** @type{number} */ x0 = region[i + 1];
      let /** @type{number} */ x1 = region[i + 2];
      min_y = min_y < y ? min_y : y;
      max_y = max_y > y ? max_y : y;
      min_x = min_x < x0 ? min_x : x0;
      max_x = max_x > x1 ? max_x : x1;
    }

    let /** @type{number} */ dx = max_x - min_x;
    let /** @type{number} */ dy = max_y + 1 - min_y;
    let /** @type{number} */ d = dx * dx + dy * dy;
    for (let /** @type{number} */ i = 0; i < MAX_LEVEL; ++i) {
      if (d >= this.levelScale[i]) {
        return i;
      }
    }
    return MAX_LEVEL - 1;
  }
}

/**
 * @param{number} v
 * @param{number} q
 * @return{number}
 */
export let dequantizeColor = (v, q) => ((255 * v + q - 2) / (q - 1)) | 0;

/** @const @type{number} */
CodecParams.MAX_CODE = MAX_PARTITION_CODE * MAX_COLOR_CODE;

/** @const @type{number} */
CodecParams.NODE_FILL = 0;
/** @const @type{number} */
CodecParams.NODE_HALF_PLANE = 1;
/** @const @type{number} */
CodecParams.NODE_TYPE_COUNT = 2;
