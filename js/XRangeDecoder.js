import {b8, b11, b11m, b16} from "./Mini.js";

// See Java sources for magic values explanation.

/**
 * @return {number}
 */
let readBit = () => {
  let /** @type{number} */ byte = _data[_pos >> 3];
  return (byte >> (_pos++ & 7)) & 1;
};

/** @type{number} */
let  _state;
/** @type{number} */
let _pos;
/** @type{!Uint8Array} */
let _data;

/**
 * @param{!Uint8Array} data
 * @return {void}
 */
export let init = (data) => {
  _data = data;
  //_state = 1;
  //_pos = 0;
  //for (let /** @type{number} */ i = 0; i < 16; ++i) _state = (_state * 2) + readBit();
  _state = ((_data[0] + _data[1] * b8) | 0) + b16;
  _pos = 16;
};

/**
 * @param{number} max
 * @return{number}
 */
export let readNumber = (max) => {
  if (max < 2) return 0;
  let /** @type{number} */ offset = _state & b11m;
  let /** @type{number} */ result = (offset * max + max - 1) >> 11;
  let /** @type{number} */ low = result * b11;
  let /** @type{number} */ base = (low / max) | 0;
  let /** @type{number} */ freq = (((low + b11) / max) | 0) - base;
  _state = freq * (_state >> 11) + offset - base;
  while (_state < b16) _state = (_state * 2) | readBit();
  return result;
};
