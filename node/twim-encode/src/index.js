/**
 * The input image.
 *
 * Currently image could only be specified with width, height and RGBA array.
 * Later more options will be added.
 *
 * @typedef {Object} Image
 * @property {!Uint8Array} rgba pixel data
 * @property {number} width width in pixels
 * @property {number} height height in pixels
 */

/**
 * The encoder options.
 *
 * @typedef {Object} Variant
 * @property {number} partitionCode partition strategy
 * @property {number} lineLimit partition constraint
 * @property {number[]} colorOptions color quantization / palette size options
 */

/**
 * The encoder output.
 *
 * @typedef {Object} Result
 * @property {number} mse expected mean-square-error
 * @property {?Uint8Array} data output bytes, null in case of error
 */

/**
 * Encoder the image.
 *
 * @param {!Image} image original image
 * @param {number} targetSize desired size of encoded image
 * @param {!Variant[]} variants encoding options to try
 * @returns {!Result} result
 */
let twimEncode = (() => {
  let simdCode = Uint32Array.from([1836278016, 1, 1610679297, 33751040, 201981953, 1090521601, 4244652288, 186259985]);
  let simdWorks = WebAssembly.validate(simdCode.buffer);
  let binaryName = simdWorks ? 'simd-twim.wasm' : 'twim.wasm';
  let binaryPath = require('path').resolve(__dirname, binaryName);
  var binary = require('fs').readFileSync(binaryPath);

  let twimEncoder = null;
  (async function() {
    let { instance } = await WebAssembly.instantiate(binary, {});
    instance.exports._initialize();
    twimEncoder = instance;
  }());

  return function(image, targetSize, variants) {
    let result = {
      mse: 0,
      data: null,
    };
    return result;
  }
})();

module.exports = twimEncode;