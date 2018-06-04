goog.provide("twim.CjkEncoder");

goog.require("twim.CjkTransform");

/**
 * @param{!Array<number>} data bytes
 * @return{!Array<number>} CJK code-points
 */
twim.CjkEncoder.encode = function(data) {
  let nibbles = [];
  let offset = 0;
  while (offset < data.length * 8) {
    let p = 0;
    let ix = offset >> 3;
    for (let i = 0; i < 6; ++i) p = (p * 256) + (data[ix + 5 - i] | 0);
    p = Math.floor(p / (1 << (offset & 7))) % 2199023255552;
    offset += 41;
    for (let i = 0; i < 5; ++i) {
      nibbles.push(p % 296);
      p = Math.floor(p / 296);
    }
  }
  /** @type{!Array<number>} */
  let result = [];
  for (let i = 0; i < nibbles.length; i += 2) {
    let ord = (nibbles[i] | 0) + 296 * (nibbles[i + 1] | 0);
    result.push(twim.CjkTransform.ordinalToUnicode(ord));
  }
  let unicodeZero = twim.CjkTransform.ordinalToUnicode(0);
  while (result[result.length - 1] === unicodeZero) {
    result.pop();
  }
  return result;
};
