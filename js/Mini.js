/**
 * @noinline
 * @param{number|!Array} n
 * @return {!Int32Array}
 */
export let newInt32Array = (n) => new Int32Array(n);

/**
 * @noinline
 * @param{!Int32Array} dst
 * @param{!Array} src
 * @param{number} offset
 * @return {void}
 */
export let int32ArraySet = (dst, src, offset) => dst.set(src, offset);

/**
 * @noinline
 * @param{!Array|!Int32Array|!Uint8Array} a
 * @return {number}
 */
export let last = (a) =>  a.length - 1;

export let /** @const */ math = Math;

export let /** @const @noinline @type{number} */ b8 = 256;
export let /** @const @noinline @type{number} */ b11 = b8 * 8;
export let /** @const @noinline @type{number} */ b11m = b11 - 1;
export let /** @const @noinline @type{number} */ b16 = b8 * b8;
export let /** @const @type{number} */ b32 = b16 * b16;
export let /** @const @type{number} */ b40 = b32 * b8;
export let /** @const @type{number} */ b48 = b40 * b8;

export let /** @type{function(number):number} */ mathFloor = math.floor;
export let /** @type{function(number):number} */ mathRound = math.round;

/**
 * @param{!Int32Array} region
 * @param{function(number, number, number)} callback
 * @return {void}
 */
export let forEachScan = (region, callback) => {
  for (let /** @type{number} */ i = region[last(region)] * 3 - 3; i >= 0; i -= 3) {
    callback(region[i], region[i + 1], region[i + 2]);
  }
};

/**
 * @noinline
 * @param{boolean} b
 * @return {void}
 */
export let assertFalse = b => {if (b) throw 4;};
