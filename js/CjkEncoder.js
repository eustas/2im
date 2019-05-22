goog.provide('twim.CjkEncoder');

goog.require('twim.CjkTransform');

/**
 * @param{!Uint8Array} data bytes
 * @return{!Array<number>} CJK code-points
 */
twim.CjkEncoder.encode = function(data) {
  let /** @type{number} */ dataBits = data.length * 8;
  let /** @type{!Uint16Array} */ nibbles = new Uint16Array(5 * Math.ceil(dataBits / 41) + 1);
  let /** @type{number} */ offset = 0;
  let /** @type{number} */ nibblesLength = 0;
  while (offset < dataBits) {
    let /** @type{number} */ p = 0;
    let /** @type{number} */ ix = offset >> 3;
    for (let /** @type{number} */ i = ix + 5; i >= ix; --i) {
      p = (p * 256) + ((i < data.length) ? data[i] : 0);
    }
    p = Math.floor(p / (1 << (offset & 7))) % 2199023255552;
    offset += 41;
    for (let /** @type{number} */ i = 0; i < 5; ++i) {
      nibbles[nibblesLength++] = p % 296;
      p = Math.floor(p / 296);
    }
  }
  let /** @type{!Array<number>} */ result = [];
  for (let /** @type{number} */ i = 0; i < nibblesLength; i += 2) {
    let /** @type{number} */ ord = nibbles[i] + 296 * nibbles[i + 1];
    result.push(twim.CjkTransform.ordinalToUnicode(ord));
  }
  let /** @type{number} */ unicodeZero = twim.CjkTransform.ordinalToUnicode(0);
  while (result[result.length - 1] === unicodeZero) {
    result.pop();
  }
  return result;
};
