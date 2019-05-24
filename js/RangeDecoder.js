// See Java sources for magic values explanation.

export class RangeDecoder {
  /**
   * @param{!Uint8Array} data bytes
   */
  constructor(data) {
    /** @private{number} */
    this._low = 0;
    /** @private{number} */
    this._range = 281474976710655;
    /** @const @private{!Uint8Array} */
    this._data = data;
    /** @private{number} */
    this._offset = 0;
    /** @private{number} */
    this._code = 0;
    this.init();
  }

  /**
   * @return {void}
   */
  init() {
    // TODO(eustas): move to "init"
    for (let /** @type{number} */ i = 0; i < 6; ++i) {
      let /** @type{number} */ nextByte =
          (this._offset < this._data.length) ? this._data[this._offset++] : 0;
      this._code = (this._code * 256) + nextByte;
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
      if (Math.floor(this._low / 1099511627776) !==
          Math.floor((this._low + this._range - 1) / 1099511627776)) {
        if (this._range > 4294967295) {
          break;
        }
        this._range = (281474976710656 - this._low) % 4294967296;
      }
      let /** @type{number} */ nextByte =
          (this._offset < this._data.length) ? this._data[this._offset++] : 0;
      this._code = ((this._code % 1099511627776) * 256) + nextByte;
      this._range = (this._range % 1099511627776) * 256;
      this._low = (this._low % 1099511627776) * 256;
    }
  }

  /**
   * @param{number} totalRange
   * @return{number}
   */
  currentCount(totalRange) {
    this._range = Math.floor(this._range / totalRange);
    return Math.floor((this._code - this._low) / this._range);
  }
}
