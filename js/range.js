// NUM_NIBBLES = 6
// NIBBLE_BITS = 8
// NIBBLE_MUL = 256 = (1 << NIBBLE_BITS)
// NIBBLE_MASK = 255 = (1 << NIBBLE_BITS) - 1
// VALUE_BITS = 48 = NUM_NIBBLES * NIBBLE_BITS
// VALUE_MUL = 281474976710656 = (1 << VALUE_BITS)
// VALUE_MASK = 281474976710655 = (1 << VALUE_BITS) - 1
// HEAD_NIBBLE_SHIFT = 40 = VALUE_BITS - NIBBLE_BITS
// HEAD_START = 1099511627776 = 1 << HEAD_NIBBLE_SHIFT
// RANGE_LIMIT_BITS = 32 = HEAD_NIBBLE_SHIFT - NIBBLE_BITS
// RANGE_LIMIT_MASK = 4294967295 = (1 << RANGE_LIMIT_BITS) - 1

/**
 * @param{!Array<number>} data encoded bytes
 * @param{!Array<number>} raw flattened original triplets
 * @return{number}
 */
function checkData(data, raw) {
  let d = new Decoder(data);
  for (let i = 0; i < raw.length; i += 3) {
    let c = d.currentCount(raw[i + 2]);
    if (c < raw[i]) {
      return -1;
    } else if (c >= raw[i + 1]) {
      return 1;
    }
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
    this.range = 281474976710655;
    /** @const @private{!Array<number>} */
    this.data = data;
    /** @private{number} */
    this.offset = 0;
    /** @private{number} */
    this.code = 0;
    for (let i = 0; i < 6; ++i) {
      this.code = (this.code * 256) + (this.data[this.offset++] | 0);
    }
  }

  /**
   * @param{number} bottom
   * @param{number} top
   * @return {void}
   */
  removeRange(bottom, top) {
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
    this.range = 281474976710655;
    /** @private{!Array<number>} */
    this.data = [];
  }

  /**
   * @param{number} bottom
   * @param{number} top
   * @param{number} totalRange
   * @return {void}
   */
  encodeRange(bottom, top, totalRange) {
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
  }

  /**
   * @return{!Array<number>}
   */
  finish() {
    for (let i = 0; i < 6; ++i) {
      this.data.push(Math.floor(this.low / 1099511627776) & 255);
      this.low = (this.low % 1099511627776) * 256;
    }
    return this.data;
  }
}

class OptimalEncoder {
  constructor() {
    /** @const @private{!Encoder} */
    this.encoder = new Encoder();
    /** @const @private{!Array<number>} */
    this.raw = [];
  }

  /**
   * @param{number} bottom
   * @param{number} top
   * @param{number} totalRange
   * @return {void}
   */
  encodeRange(bottom, top, totalRange) {
    this.raw.push(bottom, top, totalRange);
    this.encoder.encodeRange(bottom, top, totalRange);
  }

  /**
   * @return{!Array<number>}
   */
  finish() {
    let data = this.encoder.finish();
    while (true) {
      if (data.length === 0) {
        return data;
      }
      let lastByte = data.pop();
      if ((lastByte === 0) || (checkData(data, this.raw) === 0)) {
        continue;
      }
      let offset = data.length - 1;
      while (offset >= 0 && data[offset] === 255) {
        offset--;
      }
      if (offset < 0) {
        data.push(lastByte);
        return data;
      }
      data[offset]++;
      for (let i = offset + 1; i < data.length; ++i) {
        data[i] = 0;
      }
      if (checkData(data, this.raw) === 0) continue;
      data[offset]--;
      for (let i = offset + 1; i < data.length; ++i) {
        data[i] = 255;
      }
      data.push(lastByte);
      return data;
    }
  }
}

/** @export */
const range = {};
range.Decoder = Decoder;
range.Encoder = Encoder;
range.OptimalEncoder = OptimalEncoder;
