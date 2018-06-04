goog.provide("twim.RangeEncoder");

// See Java sources for magic values explanation.

/**
 * @constructor
 */
twim.RangeEncoder = function() {
  /** @private{number} */
  this.low = 0;
  /** @private{number} */
  this.range = 281474976710655;
  /**
   * @const
   * @private{!Array<number>}
   */
  this.data = [];
};

/**
 * @param{number} bottom
 * @param{number} top
 * @param{number} totalRange
 * @return {void}
 */
twim.RangeEncoder.prototype.encodeRange = function(bottom, top, totalRange) {
  this.range = Math.floor(this.range / totalRange);
  this.low += bottom * this.range;
  this.range *= top - bottom;
  while (true) {
    if (Math.floor(this.low / 1099511627776) !== Math.floor((this.low + this.range - 1) / 1099511627776)) {
      if (this.range > 4294967295) {
        break;
      }
      this.range = (281474976710656 - this.low) % 4294967296;
    }
    this.data.push(Math.floor(this.low / 1099511627776) & 255);
    this.range = (this.range % 1099511627776) * 256;
    this.low = (this.low % 1099511627776) * 256;
  }
};

/**
 * @return{!Array<number>}
 */
twim.RangeEncoder.prototype.finish = function() {
  for (let i = 0; i < 6; ++i) {
    this.data.push(Math.floor(this.low / 1099511627776) & 255);
    this.low = (this.low % 1099511627776) * 256;
  }
  return this.data;
};
