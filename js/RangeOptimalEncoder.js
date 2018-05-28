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

class OptimalEncoder {
  constructor() {
    /**
     * @const
     * @private{!Encoder}
     */
    this.encoder = new RangeEncoder();
    /**
     * @const
     * @private{!Array<number>}
     */
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
const RangeOptimalEncoder = OptimalEncoder;
