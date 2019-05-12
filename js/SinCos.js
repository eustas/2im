goog.provide('twim.SinCos');

/** @const @type{number} */
twim.SinCos.SCALE_BITS = 20;

/** @const @type{number} */
twim.SinCos.SCALE = 1 << twim.SinCos.SCALE_BITS;

/** @const @type{number} */
twim.SinCos.MAX_ANGLE_BITS = 9;

/** @const @type{number} */
twim.SinCos.MAX_ANGLE = 1 << twim.SinCos.MAX_ANGLE_BITS;

let /** @const @type{number} */ SCALE = twim.SinCos.SCALE;
let /** @const @type{number} */ MAX_ANGLE = twim.SinCos.MAX_ANGLE;
let /** @const @type{!Int32Array} */ SIN = new Int32Array(MAX_ANGLE);
let /** @const @type{!Int32Array} */ COS = new Int32Array(MAX_ANGLE);

for (let i = 0; i < twim.SinCos.MAX_ANGLE; ++i) {
  SIN[i] = Math.round(SCALE * Math.sin((Math.PI * i) / MAX_ANGLE)) | 0;
  COS[i] = Math.round(SCALE * Math.cos((Math.PI * i) / MAX_ANGLE)) | 0;
}

/** @const @type{!Int32Array} */
twim.SinCos.SIN = SIN;

/** @const @type{!Int32Array} */
twim.SinCos.COS = COS;
