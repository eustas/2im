/**
 * @const @type{!Uint32Array}
 * @private
 */
const CJK_BLOCKS = new Uint32Array([
  0x04E00, 0x09FFF,  // CJK Unified Ideographs
  0x03400, 0x04DBF,  // CJK Unified Ideographs Extension A
  0x20000, 0x2A6DF,  // CJK Unified Ideographs Extension B
  0x2A700, 0x2B73F,  // CJK Unified Ideographs Extension C
  0x2B740, 0x2B81F,  // CJK Unified Ideographs Extension D
  0x2B820, 0x2CEAF,  // CJK Unified Ideographs Extension E
  0x2CEB0, 0x2EBEF   // CJK Unified Ideographs Extension F
]);

/**
 * @param{number} chr
 * @returns {number}
 */
export function unicodeToOrdinal(chr) {
  let /** @type{number} */ sum = 0;
  for (let /** @type{number} */ i = 0; i < CJK_BLOCKS.length; i += 2) {
    if (chr >= CJK_BLOCKS[i] && chr <= CJK_BLOCKS[i + 1]) {
      return sum + chr - CJK_BLOCKS[i];
    }
    sum += CJK_BLOCKS[i + 1] - CJK_BLOCKS[i] + 1;
  }
  return -1;
}

/**
 * @param{number} ord
 * @returns {number}
 */
export function ordinalToUnicode(ord) {
  let /** @type{number} */ sum = ord;
  for (let /** @type{number} */ i = 0; i < CJK_BLOCKS.length; i += 2) {
    let /** @type{number} */ size = CJK_BLOCKS[i + 1] - CJK_BLOCKS[i] + 1;
    if (sum < size) {
      return sum + CJK_BLOCKS[i];
    }
    sum -= size;
  }
  return -1;
}
