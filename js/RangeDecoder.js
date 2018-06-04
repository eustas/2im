goog.provide("twim.RangeDecoder");

// See Java sources for magic values explanation.

/**
 * @param{!Array<number>} data bytes
 * @constructor
 */
twim.RangeDecoder = function(data) {
  /** @private{number} */
  this.low = 0;
  /** @private{number} */
  this.range = 281474976710655;
  /**
   * @const
   * @private{!Array<number>}
   */
  this.data = data;
  /** @private{number} */
  this.offset = 0;
  /** @private{number} */
  this.code = 0;
  for (let i = 0; i < 6; ++i) {
    this.code = (this.code * 256) + (this.data[this.offset++] | 0);
  }
};

/**
 * @param{number} bottom
 * @param{number} top
 * @return {void}
 */
twim.RangeDecoder.prototype.removeRange = function(bottom, top) {
  this.low += bottom * this.range;
  this.range *= top - bottom;
  while (true) {
    if (Math.floor(this.low / 1099511627776) !== Math.floor((this.low + this.range - 1) / 1099511627776)) {
      if (this.range > 4294967295) {
        break;
      }
      this.range = (281474976710656 - this.low) % 4294967296;
    }
    this.code = ((this.code % 1099511627776) * 256) + (this.data[this.offset++] | 0);
    this.range = (this.range % 1099511627776) * 256;
    this.low = (this.low % 1099511627776) * 256;
  }
};

/**
 * @param{number} totalRange
 * @return{number}
 */
twim.RangeDecoder.prototype.currentCount = function(totalRange) {
  this.range = Math.floor(this.range / totalRange);
  return Math.floor((this.code - this.low) / this.range);
};
