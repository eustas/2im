const /** @type{number} */ SCALE_BITS = 20;
export const /** @type{number} */ SCALE = 1 << SCALE_BITS;
const /** @type{number} */ MAX_ANGLE_BITS = 9;
export const /** @type{number} */ MAX_ANGLE = 1 << MAX_ANGLE_BITS;

export const /** @const @type{!Int32Array} */ SIN = new Int32Array(MAX_ANGLE);
export const /** @const @type{!Int32Array} */ COS = new Int32Array(MAX_ANGLE);

for (let /** @type{number} */ i = 0; i < MAX_ANGLE; ++i) {
  SIN[i] = Math.round(SCALE * Math.sin((Math.PI * i) / MAX_ANGLE)) | 0;
  COS[i] = Math.round(SCALE * Math.cos((Math.PI * i) / MAX_ANGLE)) | 0;
}
