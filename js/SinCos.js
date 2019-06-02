import {math, mathRound, newInt32Array, pow2} from "./Mini.js";

const /** @type{number} */ SCALE_BITS = 18;
export let /** @noinline @type{number} */ SCALE = pow2(SCALE_BITS);
const /** @type{number} */ MAX_ANGLE_BITS = 9;
export let /** @noinline @type{number} */ MAX_ANGLE = pow2(MAX_ANGLE_BITS);

export let /** @const @type{!Int32Array} */ SIN = newInt32Array(MAX_ANGLE);
export let /** @const @type{!Int32Array} */ COS = newInt32Array(MAX_ANGLE);

for (let /** @type{number} */ i = 0; i < MAX_ANGLE; ++i) {
  let /** @type{number} */ angle = (math.PI * i) / MAX_ANGLE;
  SIN[i] = mathRound(SCALE * math.sin(angle));
  COS[i] = mathRound(SCALE * math.cos(angle));
}
