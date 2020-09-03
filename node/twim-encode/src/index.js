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
 * @callback SetTatgetSize specify desired output size in bytes
 *
 * @param {number} targetSize target outpus size
 */

/**
 * @callback SetVariants specify encoder settings to try during encoding
 *
 * @param {Variaant[]} variants settings array
 */

/**
 * @callback EncodeImage encode image (with previously set options)
 *
 * @param {Image} original image
 * @result {Result} encoder output
 */

/**
 * @callback CreateEncoder creates the encoder instance
 *
 * It is recommended to reuse instance. However, concurrent usage will likely
 * "Break" the instance.
 *
 * If any instance method throws an error, then instance is considered "broken"
 * and should not be used any longer.
 * 
 * @return {Encoder} new encoder instance
 */

/**
 * Encoder instance.
 *
 * @typedef {Object} Encoder
 * @property {SetTatgetSize} setTargetSize 
 * @property {SetVariants} setVariants
 * @property {EncodeImage} encodeImage
 */

module.exports = /** @type {CreateEncoder} */ async () => {
  let encoder_ = null;
  let targetSize_ =  0;
  let variants_ = 0;
  let numVariants_ = 0;

  let wrap = (fn) => {
    if (encoder_) {
      try {
        fn(encoder_);
      } catch (ex) {
        console.log(ex);
        encoder_ = null;
      }
    }
    if (!encoder_) throw "twim encoder is broken";
  }

  let setTargetSize = (targetSize) => {
    targetSize_ = targetSize | 0;
  };

  let invArgEx = "invalid argument";

  let setVariants = (variants) => {
    let n = variants.length | 0;
    numVariants_ = n;
    if (!n) throw invArgEx;
    let bytes = new Uint8Array(10 * n);
    for (let i = 0; i < n; ++i) {
      let variant = variants[i];
      let offset = i * 10;
      bytes[offset + 0] = variant.partitionCode & 0xFF;
      bytes[offset + 1] = variant.partitionCode >> 8;
      bytes[offset + 2] = variant.lineLimit;
      let colorOptions = variant.colorOptions;
      let m = colorOptions.length | 0;
      if (!m) throw invArgEx;
      for (let j = 0; j < m; ++j) {
        let option = colorOptions[j];
        if (option >= 0 && option < 17 + 32) {
          bytes[offset + 3 + (option >> 3)] |= 1 << (option & 7);
        } else {
          throw invArgEx;
        }
      }
    }
    wrap((e) => {
      if (variants_) e.free(variants_);
      variants_ = e.malloc(10 * n);
      new Uint8Array(e.memory.buffer).set(bytes, variants_);
    });
  };

  let encodeImage = (image) => {
    let result = {
      mse: 0,
      data: null,
    };
    let w = image.width | 0;
    let h = image.height | 0;
    if (targetSize_ < 4 || !numVariants_ || w < 8 || h < 8) return result;
    wrap((e) => {
      let pixels = e.malloc(w * h * 4);
      new Uint8Array(e.memory.buffer).set(image.rgba, pixels);
      let data = e.twimEncode(w, h, pixels, targetSize_, numVariants_, variants_);
      if (data) {
        let m = e.memory.buffer;
        let size = new Uint32Array(m)[data >> 2];
        result.mse = new Float32Array(m)[(data + 4) >> 2];
        result.data = Uint8Array.from(new Uint8Array(m, data + 8, size));
        e.free(data);
      }
    });
    return result;
  };

  let initialize = async () => {
    let simdCode = Uint32Array.of(1836278016, 1, 1610679297, 33751040, 201981953, 1090521601, 4244652288, 186259985);
    let simdWorks = WebAssembly.validate(simdCode.buffer);
    let binaryName = (simdWorks ? 'simd-' : '') + 'twim.wasm';
    let binaryPath = require('path').resolve(__dirname, binaryName);
    let binary = require('fs').readFileSync(binaryPath);
    let env = {
      debugInt: (x) => {console.log("i " + x);},
      debugFloat: (x) => {console.log("f " + x);},
    };
    let bundle = await WebAssembly.instantiate(binary, {"env": env});
    encoder_ = bundle.instance.exports;
    encoder_._initialize();
  };
  await initialize();
  return {
    setTargetSize: setTargetSize,
    setVariants: setVariants,
    encodeImage: encodeImage,
  };
};
