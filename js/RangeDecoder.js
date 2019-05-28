import {b8, b32, b40, b48, mathFloor} from "./Mini.js";

// See Java sources for magic values explanation.

/**
 * @param{!RangeDecoder} self
 * @return {number}
 */
let nextByte = (self) => self._data[self._offset++] | 0;

/**
 * @param{!RangeDecoder} self
 * @return {void}
 */
let init = (self) => {
  for (let /** @type{number} */ i = 0; i < 6; ++i) {
    self._code = (self._code * b8) + nextByte(self);
  }
};

/**
 * @constructor
 * @param{!Uint8Array} data bytes
 */
export function RangeDecoder(data) {
  /** @private{number} */
  this._low = 0;
  /** @private{number} */
  this._range = b48 - 1;
  /** @const @private{!Uint8Array} */
  this._data = data;
  /** @private{number} */
  this._offset = 0;
  /** @private{number} */
  this._code = 0;
  init(this);
}

/**
 * @param{!RangeDecoder} self
 * @param{number} bottom
 * @param{number} top
 * @return {void}
 */
export let removeRange = (self, bottom, top) => {
  self._low += bottom * self._range;
  self._range *= top - bottom;
  while (true) {
    if (mathFloor(self._low / b40) !==
      mathFloor((self._low + self._range - 1) / b40)) {
      if (self._range >= b32) {
        break;
      }
      self._range = (b48 - self._low) % b32;
    }
    self._code = ((self._code % b40) * b8) + nextByte(self);
    self._range = (self._range % b40) * b8;
    self._low = (self._low % b40) * b8;
  }
};

/**
 * @param{!RangeDecoder} self
 * @param{number} totalRange
 * @return{number}
 */
export let currentCount = (self, totalRange) => {
  self._range = mathFloor(self._range / totalRange);
  let result = mathFloor((self._code - self._low) / self._range);
  if (result < 0 || result > totalRange) throw 3;
  return result;
};
