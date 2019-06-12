import * as CodecParams from "./CodecParams.js";
import {getWidth, getHeight} from "./CodecParams.js";
import * as RangeDecoder from "./RangeDecoder.js";
import {readNumber} from "./RangeDecoder.js";
import {b8, forEachScan, int32ArraySet, last, newInt32Array} from './Mini.js';
import * as DistanceRange from "./DistanceRange.js";
import * as Region from "./Region.js";
import * as SinCos from './SinCos.js';

/**
 * @noinline
 * @param{number} q
 * @return{number}
 */
let readColor = (q) => CodecParams.dequantizeColor(readNumber(q), q);

/**
 * @return{!Array<number>}
 */
let parseColor = () => [
  readColor(CodecParams.getColorQuant()),
  readColor(CodecParams.getColorQuant()),
  readColor(CodecParams.getColorQuant()),
  b8 - 1
];

/**
 * @param{!Int32Array} region
 * @param{!Array<!Int32Array>} children
 * @param{number} width
 * @param{!Uint8ClampedArray} rgba
 * @return{void}
 */
let parse = (region, children, width, rgba) => {
  let /** @type{number} */ type = readNumber(CodecParams.NODE_TYPE_COUNT);

  if (type == CodecParams.NODE_FILL) {
    let /** @type{!Array<number>} */ color = parseColor();
    forEachScan(region, (y, x0, x1) => {
      for (let /** @type{number} */ x = x0; x < x1; ++x) {
        rgba.set(color, (y * width + x) * 4);
      }
    });
    return;
  }

  let /** @type{number} */ level = CodecParams.getLevel(region);

  let /** @type{number} */ lastCount3_1 = region[last(region)] * 3 + 1;
  let /** @type{!Int32Array} */ inner = newInt32Array(lastCount3_1);
  let /** @type{!Int32Array} */ outer = newInt32Array(lastCount3_1);

  // type === CodecParams.NODE_HALF_PLANE
  let /** @type{number} */ angleMax = 1 << CodecParams.angleBits[level];
  let /** @type{number} */ angleMult = (SinCos.MAX_ANGLE / angleMax);
  let /** @type{number} */ angleCode = readNumber(angleMax);
  let /** @type{number} */ angle = angleCode * angleMult;
  DistanceRange.update(region, angle);
  let /** @type{number} */ line = readNumber(DistanceRange.getNumLines());
  Region.splitLine(region, angle, DistanceRange.distance(line), inner, outer);

  children.push(inner, outer);
};

/**
 * @noinline
 * @param{!Uint8Array} encoded
 * @return{!ImageData}
 */
export let decode = (encoded) => {
  RangeDecoder.init(encoded);
  CodecParams.read();

  let /** @type{!Int32Array} */ rootRegion = newInt32Array(getHeight() * 3 + 1);
  rootRegion[last(rootRegion)] = getHeight();
  for (let /** @type{number} */ y = 0; y < getHeight(); ++y) {
    int32ArraySet(rootRegion, [y, 0, getWidth()], y * 3);
  }

  let /** @type{!Uint8ClampedArray} */ rgba = new Uint8ClampedArray(4 * getWidth() * getHeight());
  let /** @type{!Array<!Int32Array>} */ children = [rootRegion];
  let /** @type{number} */ cursor = 0;
  while (cursor <= last(children)) {
    let /** @type{number} */ checkpoint = last(children);
    for (; cursor <= checkpoint; ++cursor) {
      parse(children[cursor], children, getWidth(), rgba);
    }
  }

  return new ImageData(rgba, getWidth(), getHeight());
};
