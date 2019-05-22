goog.provide('twim.CjkDecoder');

goog.require('twim.CjkTransform');

/**
 * @param{!Array<number>} data CKJ code-points
 * @return{!Uint8Array} bytes
 */
twim.CjkDecoder.decode = function(data) {
  let /** @type{number} */ nibblesLength = data.length * 2;
  let /** @type{!Uint16Array} */ nibbles = new Uint16Array(5 * Math.ceil(nibblesLength / 5) + 5);
  for (let /** @type{number} */ i = 0; i < data.length; ++i) {
    let /** @type{number} */ ord = twim.CjkTransform.unicodeToOrdinal(data[i] | 0);
    nibbles[2 * i] = ord % 296;
    nibbles[2 * i + 1] = ord / 296;
  }
  let /** @type{!Uint8Array} */ result = new Uint8Array(nibblesLength + Math.floor(nibblesLength / 38) + 6);
  let /** @type{number} */ resultLength = 0;
  let /** @type{number} */ ix = 0;
  let /** @type{number} */ acc = 0;
  let /** @type{number} */ bits = 0;
  while (ix < nibblesLength || acc !== 0) {
    if (bits < 8) {
      let /** @type{number} */ p = 0;
      for (let /** @type{number} */ j = 0; j < 5; ++j) {
        p = (p * 296) + (nibbles[ix + 4 - j] | 0);
      }
      ix += 5;
      acc += p * (1 << bits);
      bits += 41;
    }
    result[resultLength++] = acc & 0xFF;
    acc = Math.floor(acc / 256);
    bits -= 8;
  }
  while (result[resultLength - 1] === 0) {
    resultLength--;
  }
  return result.subarray(0, resultLength);
};
