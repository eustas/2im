import {b8, b16, math, mathRound, newInt32Array} from "./Mini.js";

export let /** @noinline @type{number} */ SCALE = b16 * 4;
export const /** @type{number} */ MAX_ANGLE_BITS = 9;
export let /** @noinline @type{number} */ MAX_ANGLE = b8 * 2;

export let /** @const @type{!Int32Array} */ SIN = newInt32Array(MAX_ANGLE);
export let /** @const @type{!Int32Array} */ COS = newInt32Array(MAX_ANGLE);

for (let /** @type{number} */ i = 0; i < MAX_ANGLE; ++i) {
  let /** @type{number} */ angle = (math.PI * i) / MAX_ANGLE;
  SIN[i] = mathRound(SCALE * math.sin(angle));
  COS[i] = mathRound(SCALE * math.cos(angle));
}
