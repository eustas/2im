/** @const */ var kSize = 6;
/** @const */ var kBase = 256;
/** @const */ var kBaseMask = kBase - 1;
/** @const */ var kMaxRange = 16777216;
/** @const */ var kMaxRangeMask = kMaxRange - 1;
/** @const */ var kShift = 1099511627776;

/**
 * @param{!Array<number>} data encoded bytes
 * @param{!Array<number>} raw flattened original triplets
 * @return{number}
 */
function testData(data, raw) {
  var d = new Decoder(data);
  for (var i = 0; i < raw.length; i += 3) {
    var c = d.currentCount(raw[i + 2]);
    if (c < raw[i]) return -1;
    if (c >= raw[i + 1]) return 1;
    d.removeRange(raw[i], raw[i + 1]);
  }
  return 0;
}

class Decoder {
  /**
   * @param{!Array<number>} data bytes
   */
  constructor(data) {
    /** @private{number} */
    this.low = 0;
    /** @private{number} */
    this.range = 281474976710656;
    /** @private{!Array<number>} */
    this.data = data;
    /** @private{number} */
    this.offset = 0;
    /** @private{number} */
    this.code = 0;
    for (var i = 0; i < kSize; ++i) {
      this.code = (this.code * kBase) + (this.data[this.offset++] | 0);
    }
  }

  /**
   * @param{number} bottom
   * @param{number} top
   */
  removeRange(bottom, top) {
    this.low = this.low + bottom * this.range;
    this.range = this.range * (top - bottom);
    while (this.range < kShift) {
      if (Math.floor(this.low / kShift) != Math.floor((this.low + this.range - 1) / kShift)) {
        if (this.range >= kMaxRange) break;
        this.range = kMaxRangeMask - (this.low % kMaxRange);
      }
      this.code = ((this.code % kShift) * kBase) + (this.data[this.offset++] | 0);
      this.range = this.range * kBase;
      this.low = (this.low % kShift) * kBase;
    }
  }

  /**
   * @param{number} totalRange
   * @return{number}
   */
  currentCount(totalRange) {
    this.range = Math.floor(this.range / totalRange);
    return Math.floor((this.code - this.low) / this.range);
  }
}

class Encoder {
  constructor() {
    /** @private{number} */
    this.low = 0;
    /** @private{number} */
    this.range = 281474976710656;
    /** @private{!Array<number>} */
    this.data = [];
  }

  /**
   * @param{number} bottom
   * @param{number} top
   * @param{number} totalRange
   */
  encodeRange(bottom, top, totalRange) {
    this.range = Math.floor(this.range / totalRange);
    this.low = this.low + bottom * this.range;
    this.range = this.range * (top - bottom);
    while (this.range < kShift) {
      if (Math.floor(this.low / kShift) != Math.floor((this.low + this.range - 1) / kShift)) {
        if (this.range >= kMaxRange) break;
        this.range = kMaxRangeMask - (this.low % kMaxRange);
      }
      this.data.push(Math.floor(this.low / kShift) & kBaseMask);
      this.range = this.range * kBase;
      this.low = (this.low % kShift) * kBase;
    }
  }

  /**
   * @return{!Array<number>}
   */
  finish() {
    for (var i = 0; i < kSize; ++i) {
      this.data.push(Math.floor(this.low / kShift) & kBaseMask);
      this.low = (this.low % kShift) * kBase;
    }
    return this.data;
  }
}

class OptimalEncoder {
  constructor() {
    /** @private{!Encoder} */
    this.encoder = new Encoder();
    /** @private{!Array<number>} */
    this.raw = [];
  }

  /**
   * @param{number} bottom
   * @param{number} top
   * @param{number} totalRange
   */
  encodeRange(bottom, top, totalRange) {
    this.raw.push(bottom, top, totalRange);
    this.encoder.encodeRange(bottom, top, totalRange);
  }

  /**
   * @return{!Array<number>}
   */
  finish() {
    var data = this.encoder.finish();
    //if (testData(data, this.raw) != 0) throw "invalid state";
    while (true) {
      if (data.length == 0) return data;
      var lastByte = data.pop();
      if (lastByte == 0) continue;
      var delta = testData(data, this.raw);
      if (delta == 0) continue;
      //if (delta > 0) throw "invalid state";
      var offset = data.length - 1;
      while (offset >= 0 && data[offset] == 255) offset--;
      if (offset < 0) {
        data.push(lastByte);
        return data;
      }
      data[offset]++;
      for (var i = offset + 1; i < data.length; ++i) data[i] = 0;
      delta = testData(data, this.raw);
      if (delta == 0) continue;
      data[offset]--;
      for (var i = offset + 1; i < data.length; ++i) data[i] = 255;
      data.push(lastByte);
      return data;
    }
  }
}

exports.Decoder = Decoder;
exports.Encoder = Encoder;
exports.OptimalEncoder = OptimalEncoder;
