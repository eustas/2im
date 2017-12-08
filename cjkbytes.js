/** @const @type{!Array<number>} */ var CJK = [
  0x04E00, 0x09FFF, 0x03400, 0x04DBF, 0x20000, 0x2A6DF, 0x2A700, 0x2B73F,
  0x2B740, 0x2B81F, 0x2B820, 0x2CEAF, 0x2CEB0, 0x2EBEF];

/**
 * @param{!Array<number>} data CKJ charcodes
 * @return{!Array<number>} bytes
 */
function decode(data) {
  var nibbles = [];
  for (var i = 0; i < data.length; ++i) {
    var chr = data[i] | 0;
    var ord = 0;
    for (var j = 0; j < CJK.length; j += 2) {
      var size = CJK[j + 1] - CJK[j] + 1;
      if (chr >= CJK[j] && chr <= CJK[j + 1]) {
        ord += chr - CJK[j];
        break;
      }
      ord += size;
    }
    nibbles.push((ord % 296) | 0);
    nibbles.push(Math.floor(ord / 296) | 0);
  }
  var result = [];
  var ix = 0;
  var acc = 0;
  var bits = 0;
  while (ix < nibbles.length || acc != 0) {
    if (bits < 8) {
      var p = 0;
      for (var j = 0; j < 5; ++j) p = (p * 296) + (nibbles[ix + 4 - j] | 0);
      ix += 5;
      acc += p * (1 << bits);
      bits += 41;
    }
    result.push(acc & 0xFF);
    acc = Math.floor(acc / 256);
    bits -= 8;
  }
  while (result[result.length - 1] == 0) result.pop();
  return result;
}

/**
 * @param{!Array<number>} data bytes
 * @return{!Array<number>} CJK charcodes
 */
function encode(data) {
  var nibbles = [];
  var offset = 0;
  while (offset < data.length * 8) {
    var p = 0;
    var ix = offset >> 3;
    for (var i = 0; i < 6; ++i) p = (p * 256) + (data[ix + 5 - i] | 0);
    p = Math.floor(p / (1 << (offset & 7))) % 2199023255552;
    offset += 41;
    for (var i = 0; i < 5; ++i) {
      nibbles.push(p % 296);
      p = Math.floor(p / 296);
    }
  }
  var result = [];
  for (var i = 0; i < nibbles.length; i += 2) {
    var ord = (nibbles[i] | 0) + 296 * (nibbles[i + 1] | 0);
    var chr = 0;
    for (var j = 0; j < CJK.length; j += 2) {
      var size = CJK[j + 1] - CJK[j] + 1;
      if (ord < size) {
        chr = ord + CJK[j];
        break;
      }
      ord -= size;
    }
    result.push(chr);
  }
  while (result[result.length - 1] == CJK[0]) result.pop();
  return result;
}

exports.decode = decode;
exports.encode = encode;
