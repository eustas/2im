/**
 * @param{!Array<number>} data bytes
 * @return{!Array<number>} CJK code-points
 */
function encode(data) {
  let cjkBlocks = CjkRanges.blocks;
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
  let result = [];
  for (let i = 0; i < nibbles.length; i += 2) {
    let ord = (nibbles[i] | 0) + 296 * (nibbles[i + 1] | 0);
    let chr = 0;
    for (let j = 0; j < cjkBlocks.length; j += 2) {
      let size = cjkBlocks[j + 1] - cjkBlocks[j] + 1;
      if (ord < size) {
        chr = ord + cjkBlocks[j];
        break;
      }
      ord -= size;
    }
    result.push(chr);
  }
  while (result[result.length - 1] === cjkBlocks[0]) {
    result.pop();
  }
  return result;
}

/** @export */
const CjkEncoder = {};
CjkEncoder.encode = encode;
