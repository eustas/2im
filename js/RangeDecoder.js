import {assertFalse, b8, b32, b40, b48, mathFloor} from "./Mini.js";

// See Java sources for magic values explanation.

/**
 * @return {number}
 */
let nextByte = () => _data[_offset++] | 0;

/** @type{number} */
let _low;
/** @type{number} */
let  _range;
/** @type{number} */
let _offset;
/** @type{number} */
let _code;
/** @type{!Uint8Array} */
let _data;

/**
 * @param{!Uint8Array} data
 * @return {void}
 */
export let init = (data) => {
  _data = data;
  _low = 0;
  _offset = 0;
  _code = 0;
  _range = b48 - 1;
  for (let /** @type{number} */ i = 0; i < 6; ++i) {
    _code = (_code * b8) + nextByte();
  }
};

/**
 * @param{number} bottom
 * @param{number} top
 * @return {void}
 */
export let removeRange = (bottom, top) => {
  _low += bottom * _range;
  _range *= top - bottom;
  while (true) {
    if (mathFloor(_low / b40) !==
      mathFloor((_low + _range - 1) / b40)) {
      if (_range >= b32) {
        break;
      }
      _range = b48 - _low;
    }
    _code = ((_code % b40) * b8) + nextByte();
    _range = (_range % b40) * b8 + b8 - 1;
    _low = (_low % b40) * b8;
  }
};

/**
 * @param{number} totalRange
 * @return{number}
 */
export let currentCount = (totalRange) => {
  _range = mathFloor(_range / totalRange);
  let result = mathFloor((_code - _low) / _range);
  assertFalse(result < 0 || result >= totalRange);
  return result;
};

/**
 * @param{number} max
 * @return{number}
 */
export let readNumber = (max) => {
  if (max < 2) return 0;
  let /** @type{number} */ result = currentCount(max);
  removeRange(result, result + 1);
  return result;
};

/**
 * @return {number}
 */
export let readSize = () => {
  let /** @type{number} */ bits = -1;
  while ((bits < 8) || (readNumber(2) > 0)) {
    bits = 8 * bits + readNumber(8) + 8;
  }
  return bits;
};
