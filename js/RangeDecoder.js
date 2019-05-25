import {b8, b32, b40, b48, mathFloor} from "./Mini.js";

// See Java sources for magic values explanation.

/**
 * @param{!RangeDecoder} self
 * @return {number}
 */
let nextByte = (self) => self._data[self._offset++] | 0;

export class RangeDecoder {
  /**
   * @param{!Uint8Array} data bytes
   */
  constructor(data) {
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
    let /** @type{!RangeDecoder} */ self = this;
    for (let /** @type{number} */ i = 0; i < 6; ++i) {
      self._code = (self._code * b8) + nextByte(self);
    }
  }

  /**
   * @param{number} bottom
   * @param{number} top
   * @return {void}
   */
  removeRange(bottom, top) {
    this._low += bottom * this._range;
    this._range *= top - bottom;
    while (true) {
      if (mathFloor(this._low / b40) !==
          mathFloor((this._low + this._range - 1) / b40)) {
        if (this._range >= b32) {
          break;
        }
        this._range = (b48 - this._low) % b32;
      }
      this._code = ((this._code % b40) * b8) + nextByte(this);
      this._range = (this._range % b40) * b8;
      this._low = (this._low % b40) * b8;
    }
  }

  /**
   * @param{number} totalRange
   * @return{number}
   */
  currentCount(totalRange) {
    this._range = mathFloor(this._range / totalRange);
    let result = mathFloor((this._code - this._low) / this._range);
    if (result < 0 || result > totalRange) throw 3;
    return result;
  }
}
