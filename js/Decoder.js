import {dequantizeColor, CodecParams, getLevel, setCode} from "./CodecParams.js";
import {currentCount, RangeDecoder, removeRange} from "./RangeDecoder.js";
import {b8, forEachScan, int32ArraySet, last, newInt32Array} from './Mini.js';
import * as Region from "./Region.js";
import {distance, update} from "./Region.js";
import * as SinCos from './SinCos.js';

let DistanceRange = Region.DistanceRange;

/**
 * @param{!RangeDecoder} src
 * @param{number} max
 * @return{number}
 */
let readNumber = (src, max) => {
  if (max === 1) return 0;
  let /** @type{number} */ result = currentCount(src, max);
  removeRange(src, result, result + 1);
  return result;
};

/**
 * @noinline
 * @param{!RangeDecoder} src
 * @param{number} q
 * @return{number}
 */
let readColor = (src, q) => dequantizeColor(readNumber(src, q), q);

/**
 * @param{!Fragment} self
 * @param{!RangeDecoder} src
 * @param{!CodecParams} cp
 * @return{void}
 */
let parseColor = (self, src, cp) => {
  int32ArraySet(self._color, [
    readColor(src, cp.colorQuant[0]),
    readColor(src, cp.colorQuant[1]),
    readColor(src, cp.colorQuant[2]),
    b8 - 1
  ], 0);
};

/**
 * @constructor
 * @param{!Int32Array} region
 */
function Fragment(region) {
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
 * @param{!Fragment} self
 * @param{!RangeDecoder} src
 * @param{!CodecParams} cp
 * @param{!Array<!Fragment>} children
 * @param{!DistanceRange} distanceRange
 * @return{void}
 */
let parse = (self, src, cp, children, distanceRange) => {
  self._type = readNumber(src, CodecParams.NODE_TYPE_COUNT);

  if (self._type === CodecParams.NODE_FILL) {
    parseColor(self, src, cp);
    return;
  }

  let /** @type{number} */ level = getLevel(cp, self._region);
  if (level < 0) {
    throw 1;
  }

  let /** @type{number} */ lastCount3 = self._region[last(self._region)] * 3;
  let /** @type{!Int32Array} */ inner = newInt32Array(lastCount3 + 1);
  let /** @type{!Int32Array} */ outer = newInt32Array(lastCount3 + 1);

  // type === CodecParams.NODE_HALF_PLANE
  let /** @type{number} */ angleMax = 1 << cp.angleBits[level];
  let /** @type{number} */ angleMult = (SinCos.MAX_ANGLE / angleMax);
  let /** @type{number} */ angleCode = readNumber(src, angleMax);
  let /** @type{number} */ angle = angleCode * angleMult;
  update(distanceRange, self._region, angle, cp);
  let /** @type{number} */ line = readNumber(src, distanceRange.numLines);
  Region.splitLine(self._region, angle, distance(distanceRange, line), inner, outer);

  children.push(self._leftChild = new Fragment(inner));
  children.push(self._rightChild = new Fragment(outer));
};

/**
 * @param{!Fragment} self
 * @param{number} width
 * @param{!Uint8ClampedArray} rgba
 */
let render = (self, width, rgba) => {
  if (self._type === CodecParams.NODE_FILL) {
    let /** @type{!Int32Array} */ region = self._region;
    forEachScan(region, (y, x0, x1) => {
      for (let /** @type{number} */ x = x0; x < x1; ++x) {
        rgba.set(self._color, (y * width + x) * 4);
      }
    });
    return;
  }
  render(/** @type{!Fragment} */ (self._leftChild), width, rgba);
  render(/** @type{!Fragment} */ (self._rightChild), width, rgba);
};

/**
 * @noinline
 * @param{!Uint8Array} encoded
 * @param{number} width
 * @param{number} height
 * @return{!ImageData}
 */
export let decode =(encoded, width, height) => {
  let /** @type{!RangeDecoder} */ src = new RangeDecoder(encoded);
  let /** @type{!CodecParams} */ cp = new CodecParams(width, height);
  setCode(cp, readNumber(src, CodecParams.MAX_CODE));

  let /** @type{!Int32Array} */ rootRegion = newInt32Array(height * 3 + 1);
  rootRegion[last(rootRegion)] = height;
  for (let /** @type{number} */ y = 0; y < height; ++y) {
    int32ArraySet(rootRegion, [y, 0, width], y * 3);
  }

  let /** @type{!Fragment} */ root = new Fragment(rootRegion);
  let /** @type{!Array<!Fragment>} */ children = [];
  children.push(root);

  let /** @type{!DistanceRange} */ distanceRange = new DistanceRange();
  let /** @type{number} */ cursor = 0;
  while (cursor <= last(children)) {
    let /** @type{number} */ checkpoint = last(children);
    for (; cursor <= checkpoint; ++cursor) {
      parse(children[cursor], src, cp, children, distanceRange);
    }
  }

  let /** @type{!Uint8ClampedArray} */ rgba = new Uint8ClampedArray(4 * width * height);
  render(root, width, rgba);
  return new ImageData(rgba, width, height);
};
