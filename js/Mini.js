/**
 * @noinline
 * @param{number|!Array} n
 * @return {!Int32Array}
 */
export let newInt32Array = (n) => new Int32Array(n);

/**
 * @noinline
 * @param{!Array|!Int32Array|!Uint8Array} a
 * @return {number}
 */
export let last = (a) =>  a.length - 1;

/**
 * @noinline
 * @param{number} n
 * @return {number}
 */
export let pow2 = n => Math.pow(2, n);

export const /** @const @type{number} */ b8 = pow2(8);
export const /** @const @type{number} */ b32 = pow2(32);
export const /** @const @type{number} */ b40 = pow2(40);
export const /** @const @type{number} */ b48 = pow2(48);

/**
 * @noinline
 * @param{number} x
 * @return {number}
 */
export let mathFloor = (x) => Math.floor(x);