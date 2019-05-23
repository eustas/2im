// See Java sources for magic values explanation.

class Decoder {
  constructor() {
    /** @private{!Uint8Array} */
    this.data = new Uint8Array([]);
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
  }

  /**
   * @param{!Uint8Array} data
   * @param{number} dataLength
   * @return {void}
   */
  init(data, dataLength) {
    this.data = data;
    this.dataLength = dataLength;
  }

  /**
   * @param{!Decoder} other
   * @return {void}
   */
  copy(other) {
    this.offset = other.offset;
    this.code = other.code;
    this.low = other.low;
    this.range = other.range;
  }

  /**
   * @param{number} totalRange
   * @param{number} bottom
   * @param{number} top
   * @return{boolean}
   */
  decodeRange(totalRange, bottom, top) {
    this.range = Math.floor(this.range / totalRange);
    let /** @type{number} */ count = Math.floor((this.code - this.low) / this.range);
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
      let /** @type{number} */ nibble =
          (this.offset < this.dataLength) ? (this.data[this.offset++] | 0) : 0;
      this.code = ((this.code % 1099511627776) * 256) + nibble;
      this.range = (this.range % 1099511627776) * 256;
      this.low = (this.low % 1099511627776) * 256;
    }
    return true;
  }
}

export class RangeEncoder {
  constructor() {
    /** @const @private{!Array<number>} */
    this.triplets = [];
  }

  /**
   * @param{number} bottom
   * @param{number} top
   * @param{number} totalRange
   * @return {void}
   */
  encodeRange(bottom, top, totalRange) {
    this.triplets.push(totalRange);
    this.triplets.push(bottom);
    this.triplets.push(top);
  }

  /**
   * @private
   * @return{!Uint8Array}
   */
  encode() {
    let /** @type{!Array<number>} */ out = [];
    let /** @type{number} */ low = 0;
    let /** @type{number} */ range = 281474976710655;
    let /** @type{number} */ tripletsSize = this.triplets.length;
    for (let /** @type{number} */ i = 0; i < tripletsSize;) {
      let /** @type{number} */ totalRange = this.triplets[i++];
      let /** @type{number} */ bottom = this.triplets[i++];
      let /** @type{number} */ top = this.triplets[i++];
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
    for (let /** @type{number} */ i = 0; i < 6; ++i) {
      out.push(Math.floor(low / 1099511627776) & 255);
      low = (low % 1099511627776) * 256;
    }
    return new Uint8Array(out);
  }

  /**
   * @param{!Uint8Array} data
   * @private
   * @return{!Uint8Array}
   */
  optimize(data) {
    let /** @type{number} */ dataLength = data.length;
    // KISS
    if (dataLength <= 6) {
      return data;
    }

    let /** @type{!Decoder} */ current = new Decoder();
    current.init(data, dataLength);
    for (let /** @type{number} */ i = 0; i < 6; ++i) {
      current.code = (current.code * 256) + (data[current.offset++] | 0);
    }
    current.range = 281474976710655;

    let /** @type{!Decoder} */ good = new Decoder();
    good.copy(current);
    let /** @type{number} */ tripletsSize = this.triplets.length;
    let /** @type{number} */ i = 0;
    while (i < tripletsSize) {
      current.decodeRange(
          this.triplets[i], this.triplets[i + 1], this.triplets[i + 2]);
      if (current.offset + 6 > dataLength) {
        break;
      }
      good.copy(current);
      i += 3;
    }

    let /** @type{number} */ bestCut = 0;
    let /** @type{number} */ bestCutDelta = 0;
    for (let /** @type{number} */ cut = 1; cut <= 6; ++cut) {
      current.dataLength = dataLength - cut;
      let /** @type{number} */ originalTail = data[current.dataLength - 1];
      for (let /** @type{number} */ delta = -1; delta <= 1; ++delta) {
        current.copy(good);
        data[current.dataLength - 1] = (originalTail + delta) & 0xFF;
        let /** @type{number} */ j = i;
        let /** @type{boolean} */ ok = true;
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
    dataLength -= bestCut;
    data[dataLength - 1] = (data[dataLength - 1] + bestCutDelta) & 0xFF;
    return data.subarray(0, dataLength);
  }

  /**
   * @return{!Uint8Array}
   */
  finish() {
    return this.optimize(this.encode());
  }
}
