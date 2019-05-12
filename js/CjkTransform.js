goog.provide('twim.CjkTransform');

/**
 * @const @type{!Array<number>}
 * @private
 */
const CJK_BLOCKS = [
  0x04E00, 0x09FFF,  // CJK Unified Ideographs
  0x03400, 0x04DBF,  // CJK Unified Ideographs Extension A
  0x20000, 0x2A6DF,  // CJK Unified Ideographs Extension B
  0x2A700, 0x2B73F,  // CJK Unified Ideographs Extension C
  0x2B740, 0x2B81F,  // CJK Unified Ideographs Extension D
  0x2B820, 0x2CEAF,  // CJK Unified Ideographs Extension E
  0x2CEB0, 0x2EBEF   // CJK Unified Ideographs Extension F
];

/**
 * @param{number} chr
 * @returns {number}
 */
twim.CjkTransform.unicodeToOrdinal = function(chr) {
  let sum = 0;
  for (let i = 0; i < CJK_BLOCKS.length; i += 2) {
    if (chr >= CJK_BLOCKS[i] && chr <= CJK_BLOCKS[i + 1]) {
      return sum + chr - CJK_BLOCKS[i];
    }
    sum += CJK_BLOCKS[i + 1] - CJK_BLOCKS[i] + 1;
  }
  return -1;
};

/**
 * @param{number} ord
 * @returns {number}
 */
twim.CjkTransform.ordinalToUnicode = function(ord) {
  let sum = ord;
  for (let i = 0; i < CJK_BLOCKS.length; i += 2) {
    let size = CJK_BLOCKS[i + 1] - CJK_BLOCKS[i] + 1;
    if (sum < size) {
      return sum + CJK_BLOCKS[i];
    }
    sum -= size;
  }
  return -1;
};
