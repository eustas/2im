/** @const @type{!Array<number>} */ const blocks = [
  0x04E00, 0x09FFF,  // CJK Unified Ideographs
  0x03400, 0x04DBF,  // CJK Unified Ideographs Extension A
  0x20000, 0x2A6DF,  // CJK Unified Ideographs Extension B
  0x2A700, 0x2B73F,  // CJK Unified Ideographs Extension C
  0x2B740, 0x2B81F,  // CJK Unified Ideographs Extension D
  0x2B820, 0x2CEAF,  // CJK Unified Ideographs Extension E
  0x2CEB0, 0x2EBEF   // CJK Unified Ideographs Extension F
];

/** @export */
const CjkRanges = {};
CjkRanges.blocks = blocks;
