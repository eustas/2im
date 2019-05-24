import {CodecParams} from "./CodecParams.js";
import {RangeDecoder} from "./RangeDecoder.js";
import {newInt32Array} from './Mini.js';
import * as Region from "./Region.js";
import * as SinCos from './SinCos.js';

const DistanceRange = Region.DistanceRange;

/**
 * @param{!RangeDecoder} src
 * @param{number} max
 * @return{number}
 */
function readNumber(src, max) {
  if (max === 1) return 0;
  let /** @type{number} */ result = src.currentCount(max);
  src.removeRange(result, result + 1);
  return result;
}

class Fragment {
  /**
   * @param{!Int32Array} region
   */
  constructor(region) {
    /** @private @type{number} */
    this._type = CodecParams.NODE_FILL;
    /** @const @private @type{!Int32Array} */
    this._region = region;
    /** @private @type{number} */
    this.r = 0;
    /** @private @type{number} */
    this.g = 0;
    /** @private @type{number} */
    this.b = 0;
    /** @private @type{number} */
    this.a = 0;
    /** @private @type{?Fragment} */
    this._leftChild = null;
    /** @private @type{?Fragment} */
    this._rightChild = null;
  }

  /**
   * @param{!RangeDecoder} src
   * @param{!CodecParams} cp
   * @return{void}
   */
  readColor(src, cp) {
    this.a = 0xFF;
    let /** @type{number} */ qr = cp.colorQuant[0];
    let /** @type{number} */ qg = cp.colorQuant[1];
    let /** @type{number} */ qb = cp.colorQuant[2];
    this.r = CodecParams.dequantizeColor(readNumber(src, qr), qr);
    this.g = CodecParams.dequantizeColor(readNumber(src, qg), qg);
    this.b = CodecParams.dequantizeColor(readNumber(src, qb), qb);
  }

  /**
   * @param{!RangeDecoder} src
   * @param{!CodecParams} cp
   * @param{!Array<!Fragment>} children
   * @param{!DistanceRange} distanceRange
   * @return{void}
   */
  parse(src, cp, children, distanceRange) {
    this._type = readNumber(src, CodecParams.NODE_TYPE_COUNT);

    if (this._type === CodecParams.NODE_FILL) {
      this.readColor(src, cp);
      return;
    }

    let /** @type{number} */ level = cp.getLevel(this._region);
    if (level < 0) {
      throw "corrupted input";
    }

    let /** @type{number} */ lastCount3 = this._region[this._region.length - 1] * 3;
    let /** @type{!Int32Array} */ inner = newInt32Array(lastCount3 + 1);
    let /** @type{!Int32Array} */ outer = newInt32Array(lastCount3 + 1);

    switch (this._type) {
      case CodecParams.NODE_HALF_PLANE: {
        let /** @type{number} */ angleMax = 1 << cp.angleBits[level];
        let /** @type{number} */ angleMult = (SinCos.MAX_ANGLE / angleMax);
        let /** @type{number} */ angleCode = readNumber(src, angleMax);
        let /** @type{number} */ angle = angleCode * angleMult;
        distanceRange.update(this._region, angle, cp.lineQuant);
        let /** @type{number} */ line = readNumber(src, distanceRange.numLines);
        Region.splitLine(this._region, angle, distanceRange.distance(line), inner, outer);
        break;
      }

      case CodecParams.NODE_FILL:
      default:
        throw "unreachable";
    }

    children.push(this._leftChild = new Fragment(inner));
    children.push(this._rightChild = new Fragment(outer));
  }

  /**
   * @param{number} width
   * @param{!Uint8ClampedArray} rgba
   */
  render(width, rgba) {
    if (this._type === CodecParams.NODE_FILL) {
      let /** @type{!Int32Array} */ region = this._region;
      let /** @type{number} */ count3 = region[region.length - 1] * 3;
      for (let /** @type{number} */ i = 0; i < count3; i += 3) {
        let /** @type{number} */ y = region[i];
        let /** @type{number} */ x0 = region[i + 1];
        let /** @type{number} */ x1 = region[i + 2];
        for (let /** @type{number} */ x = x0; x < x1; ++x) {
          let /** @type{number} */ offset = (y * width + x) * 4;
          rgba[offset] = this.r;
          rgba[offset + 1] = this.g;
          rgba[offset + 2] = this.b;
          rgba[offset + 3] = this.a;
        }
      }
      return;
    }
    this._leftChild.render(width, rgba);
    this._rightChild.render(width, rgba);
  }
}

/**
 * @param{!Uint8Array} encoded
 * @return{!ImageData}
 */
export function decode(encoded) {
  let /** @type{!RangeDecoder} */ src = new RangeDecoder(encoded);
  let /** @type{number} */ width = 64;
  let /** @type{number} */ height = 64;
  let /** @type{!CodecParams} */ cp = new CodecParams(width, height);
  cp.setCode(readNumber(src, CodecParams.MAX_CODE));

  let /** @type{!Int32Array} */ rootRegion = newInt32Array(height * 3 + 1);
  rootRegion[rootRegion.length - 1] = height;
  for (let /** @type{number} */ y = 0; y < height; ++y) {
    rootRegion[y * 3] = y;
    rootRegion[y * 3 + 1] = 0;
    rootRegion[y * 3 + 2] = width;
  }

  let /** @type{!Fragment} */ root = new Fragment(rootRegion);
  let /** @type{!Array<!Fragment>} */ children = [];
  children.push(root);

  let /** @type{!DistanceRange} */ distanceRange = new DistanceRange();
  let /** @type{number} */ cursor = 0;
  while (cursor < children.length) {
    let /** @type{number} */ checkpoint = children.length;
    for (; cursor < checkpoint; ++cursor) {
      children[cursor].parse(src, cp, children, distanceRange);
    }
  }

  let /** @type{!Uint8ClampedArray} */ rgba = new Uint8ClampedArray(4 * width * height);
  root.render(width, rgba);
  return new ImageData(rgba, width, height);
}
