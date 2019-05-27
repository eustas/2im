import {dequantizeColor, CodecParams} from "./CodecParams.js";
import {RangeDecoder} from "./RangeDecoder.js";
import {b8, last, newInt32Array} from './Mini.js';
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

/**
 * @noinline
 * @param{!RangeDecoder} src
 * @param{number} q
 * @return{number}
 */
let readColor = (src, q) => dequantizeColor(readNumber(src, q), q);

class Fragment {
  /**
   * @param{!Int32Array} region
   */
  constructor(region) {
    /** @private @type{number} */
    this._type = CodecParams.NODE_FILL;
    /** @const @private @type{!Int32Array} */
    this._region = region;
    /** @const @private @type{!Int32Array} */
    this._color = newInt32Array(4);
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
  parseColor(src, cp) {
    this._color[0] = readColor(src, cp.colorQuant[0]);
    this._color[1] = readColor(src, cp.colorQuant[1]);
    this._color[2] = readColor(src, cp.colorQuant[2]);
    this._color[3] = b8 - 1;
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
      this.parseColor(src, cp);
      return;
    }

    let /** @type{number} */ level = cp.getLevel(this._region);
    if (level < 0) {
      throw 1;
    }

    let /** @type{number} */ lastCount3 = this._region[last(this._region)] * 3;
    let /** @type{!Int32Array} */ inner = newInt32Array(lastCount3 + 1);
    let /** @type{!Int32Array} */ outer = newInt32Array(lastCount3 + 1);

    // type === CodecParams.NODE_HALF_PLANE
    let /** @type{number} */ angleMax = 1 << cp.angleBits[level];
    let /** @type{number} */ angleMult = (SinCos.MAX_ANGLE / angleMax);
    let /** @type{number} */ angleCode = readNumber(src, angleMax);
    let /** @type{number} */ angle = angleCode * angleMult;
    distanceRange.update(this._region, angle, cp);
    let /** @type{number} */ line = readNumber(src, distanceRange.numLines);
    Region.splitLine(this._region, angle, distanceRange.distance(line), inner, outer);

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
      let /** @type{number} */ count3 = region[last(region)] * 3;
      for (let /** @type{number} */ i = 0; i < count3; i += 3) {
        let /** @const @type{number} */ y = region[i];
        let /** @const @type{number} */ x0 = region[i + 1];
        let /** @const @type{number} */ x1 = region[i + 2];
        for (let /** @type{number} */ x = x0; x < x1; ++x) {
          rgba.set(this._color, (y * width + x) * 4);
        }
      }
      return;
    }
    this._leftChild.render(width, rgba);
    this._rightChild.render(width, rgba);
  }
}

/**
 * @noinline
 * @param{!Uint8Array} encoded
 * @param{number} width
 * @param{number} height
 * @return{!ImageData}
 */
export function decode(encoded, width, height) {
  let /** @type{!RangeDecoder} */ src = new RangeDecoder(encoded);
  let /** @type{!CodecParams} */ cp = new CodecParams(width, height);
  cp.setCode(readNumber(src, CodecParams.MAX_CODE));

  let /** @type{!Int32Array} */ rootRegion = newInt32Array(height * 3 + 1);
  rootRegion[last(rootRegion)] = height;
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
  while (cursor <= last(children)) {
    let /** @type{number} */ checkpoint = last(children);
    for (; cursor <= checkpoint; ++cursor) {
      children[cursor].parse(src, cp, children, distanceRange);
    }
  }

  let /** @type{!Uint8ClampedArray} */ rgba = new Uint8ClampedArray(4 * width * height);
  root.render(width, rgba);
  return new ImageData(rgba, width, height);
}
