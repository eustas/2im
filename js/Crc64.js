goog.provide('twim.Crc64');

/** @const @type{!Uint32Array} */
let CRC_64_POLY = new Uint32Array([0xD7870F42, 0xC96C5795]);

/**
 * Roll CRC64 calculation.
 *
 * CRC64(data) = finish(update((... update(init(), firstByte), ...), lastByte));
 *
 * This simple and reliable checksum is chosen to make is easy to calculate the
 * same value across the variety of languages (C++, Java, Go, ...).
 *
 * @param{!Uint32Array} crc
 * @param{number} next
 * @return{!Uint32Array}
 */
twim.Crc64.update = function(crc, next) {
  crc[2] = (crc[0] ^ next) & 0xFF;
  crc[3] = 0;
  for (let /** @type{number} */ i = 0; i < 8; ++i) {
    crc[4] = (crc[2] >>> 1) | ((crc[3] & 1) << 31);
    crc[5] = crc[3] >>> 1;
    if (crc[2] & 1) {
      crc[2] = crc[4] ^ CRC_64_POLY[0];
      crc[3] = crc[5] ^ CRC_64_POLY[1];
    } else {
      crc[2] = crc[4];
      crc[3] = crc[5];
    }
  }
  crc[4] = (crc[0] >>> 8) | ((crc[1] & 0xFF) << 24);
  crc[5] = crc[1] >>> 8;
  crc[0] = crc[2] ^ crc[4];
  crc[1] = crc[3] ^ crc[5];
  return crc;
};

/**
 * @return{!Uint32Array}
 */
twim.Crc64.init = function() {
  //                      (crc )  (c )  (d )
  return new Uint32Array([-1, -1, 0, 0, 0, 0]);
};

/**
 * @param{!Uint32Array} crc
 * @return{string}
 */
twim.Crc64.finish = function(crc) {
  crc[2] = -1;
  crc[3] = -1;
  crc[0] = crc[0] ^ crc[2];
  crc[1] = crc[1] ^ crc[3];
  let /** @type{string} */ hi = ('00000000' + crc[1].toString(16)).slice(-8);
  let /** @type{string} */ lo = ('00000000' + crc[0].toString(16)).slice(-8);
  return (hi + lo).toUpperCase();
};
