goog.provide('twim.RangeEncoder');

// See Java sources for magic values explanation.

/**
 * @constructor
 * @private
 */
let Decoder = function() {
  /** @private{!Array<number>} */
  this.data = [];
  /** @private{number} */
  this.dataLength = 0;
  /** @private{number} */
  this.offset = 0;
  /** @private{number} */
  this.code = 0;
  /** @private{number} */
  this.low = 0;
  /** @private{number} */
  this.range = 0;
};

/**
 * @param{!Array<number>} data
 * @param{number} dataLength
 * @return {void}
 */
Decoder.prototype.init = function(data, dataLength) {
  this.data = data;
  this.dataLength = dataLength;
};

/**
 * @param{!Decoder} other
 * @return {void}
 */
Decoder.prototype.copy = function(other) {
  this.offset = other.offset;
  this.code = other.code;
  this.low = other.low;
  this.range = other.range;
};

/**
 * @param{number} totalRange
 * @param{number} bottom
 * @param{number} top
 * @return{boolean}
 */
Decoder.prototype.decodeRange = function(totalRange, bottom, top) {
  this.range = Math.floor(this.range / totalRange);
  var count = Math.floor((this.code - this.low) / this.range);
  if ((count < bottom) || (count >= top)) {
    return false;
  }

  this.low += bottom * this.range;
  this.range *= top - bottom;
  while (true) {
    if (Math.floor(this.low / 1099511627776) !==
        Math.floor((this.low + this.range - 1) / 1099511627776)) {
      if (this.range > 4294967295) {
        break;
      }
      this.range = (281474976710656 - this.low) % 4294967296;
    }
    var nibble =
        (this.offset < this.dataLength) ? (this.data[this.offset++] | 0) : 0;
    this.code = ((this.code % 1099511627776) * 256) + nibble;
    this.range = (this.range % 1099511627776) * 256;
    this.low = (this.low % 1099511627776) * 256;
  }
  return true;
};

/**
 * @constructor
 */
twim.RangeEncoder = function() {
  /** @const @private{!Array<number>} */
  this.triplets = [];
};

/**
 * @param{number} bottom
 * @param{number} top
 * @param{number} totalRange
 * @return {void}
 */
twim.RangeEncoder.prototype.encodeRange = function(bottom, top, totalRange) {
  this.triplets.push(totalRange);
  this.triplets.push(bottom);
  this.triplets.push(top);
};

/**
 * @private
 * @return{!Array<number>}
 */
twim.RangeEncoder.prototype.encode = function() {
  let /** @type{!Array<number>} */ out = [];
  let low = 0;
  let range = 281474976710655;
  let tripletsSize = this.triplets.length;
  for (let i = 0; i < tripletsSize;) {
    let totalRange = this.triplets[i++];
    let bottom = this.triplets[i++];
    let top = this.triplets[i++];
    range = Math.floor(range / totalRange);
    low += bottom * range;
    range *= top - bottom;
    while (true) {
      if (Math.floor(low / 1099511627776) !==
          Math.floor((low + range - 1) / 1099511627776)) {
        if (range > 4294967295) {
          break;
        }
        range = (281474976710656 - low) % 4294967296;
      }
      out.push(Math.floor(low / 1099511627776) & 255);
      range = (range % 1099511627776) * 256;
      low = (low % 1099511627776) * 256;
    }
  }
  for (let i = 0; i < 6; ++i) {
    out.push(Math.floor(low / 1099511627776) & 255);
    low = (low % 1099511627776) * 256;
  }
  return out;
};

/**
 * @param{!Array<number>} data
 * @private
 * @return{!Array<number>}
 */
twim.RangeEncoder.prototype.optimize = function(data) {
  // KISS
  if (data.length <= 6) {
    return data;
  }

  let /** @type{!Decoder} */ current = new Decoder();
  current.init(data, data.length);
  for (let i = 0; i < 6; ++i) {
    current.code = (current.code * 256) + (data[current.offset++] | 0);
  }
  current.range = 281474976710655;

  let /** @type{!Decoder} */ good = new Decoder();
  good.copy(current);
  let tripletsSize = this.triplets.length;
  let i = 0;
  while (i < tripletsSize) {
    current.decodeRange(
        this.triplets[i], this.triplets[i + 1], this.triplets[i + 2]);
    if (current.offset + 6 > data.length) {
      break;
    }
    good.copy(current);
    i += 3;
  }

  let bestCut = 0;
  let bestCutDelta = 0;
  for (let cut = 1; cut <= 6; ++cut) {
    current.dataLength = data.length - cut;
    let originalTail = data[current.dataLength - 1];
    for (let delta = -1; delta <= 1; ++delta) {
      current.copy(good);
      data[current.dataLength - 1] = (originalTail + delta) & 0xFF;
      let j = i;
      let ok = true;
      while (ok && (j < tripletsSize)) {
        ok = current.decodeRange(
            this.triplets[j], this.triplets[j + 1], this.triplets[j + 2]);
        j += 3;
      }
      if (ok) {
        bestCut = cut;
        bestCutDelta = delta;
      }
    }
    data[current.dataLength - 1] = originalTail;
  }
  for (let j = 0; j < bestCut; ++j) data.pop();
  data[data.length - 1] = (data[data.length - 1] + bestCutDelta) & 0xFF;
  return data;
};

/**
 * @return{!Array<number>}
 */
twim.RangeEncoder.prototype.finish = function() {
  return this.optimize(this.encode());
};
