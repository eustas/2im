goog.provide('twim.CjkDecoder');

goog.require('twim.CjkTransform');

/**
 * @param{!Array<number>} data CKJ code-points
 * @return{!Array<number>} bytes
 */
twim.CjkDecoder.decode = function(data) {
  let nibbles = [];
  for (let i = 0; i < data.length; ++i) {
    let ord = twim.CjkTransform.unicodeToOrdinal(data[i] | 0);
    nibbles.push((ord % 296) | 0);
    nibbles.push(Math.floor(ord / 296) | 0);
  }
  let result = [];
  let ix = 0;
  let acc = 0;
  let bits = 0;
  while (ix < nibbles.length || acc !== 0) {
    if (bits < 8) {
      let p = 0;
      for (let j = 0; j < 5; ++j) {
        p = (p * 296) + (nibbles[ix + 4 - j] | 0);
      }
      ix += 5;
      acc += p * (1 << bits);
      bits += 41;
    }
    result.push(acc & 0xFF);
    acc = Math.floor(acc / 256);
    bits -= 8;
  }
  while (result[result.length - 1] === 0) {
    result.pop();
  }
  return result;
};
