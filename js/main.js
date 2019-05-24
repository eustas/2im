import * as CjkDecoder from './CjkDecoder.js';
import * as Decoder from './Decoder.js';

document.addEventListener('DOMContentLoaded', function() {
  let content = /** @type{!Node} */ (document.getElementById('content'));
  let input =
      /** @type {!HTMLInputElement} */ (document.getElementById('encoded'));

  let /** @type{string} */cat64 = "EfrRqKmGzw8do41mIUK2MXEuokB2Qv23fGmkx6V23x/cgbNQLPIVUun5vibQZPS9k6QfbuKsFLCHIp86oxEbj1p0PqrDD7okvBd6gOIZ7qBref6amZPSTZlnV76ttToxYJX+4fQ6tlofHJfYSoRcWYa6y4rDoBCYQhgYLMnGkQWzLHyBOwR6t7zz7Kz9J4UnVVOYPiYHXbOxHfrOR2e5t6A19xfzIasTPNHWwyA9CE3CG5sFf1FgNdrX6UewBt01SLolZv3sUVEqZuMFLQjWR676cXOZX4Jncb/q8LrOGBMYerIjv9wu/vXPgDaYHkUT86o/6cB+wmfx8zpM2oUWlmbqzbjpbS058o2C8EZY+Z8koS92GuWbPai+LUro0DqefT5nvvmOn/EIZSwep3lQ7P/qTcoYmTKXiD1dix23fmT90QqUr5ie";
  let /** @type{string} */ catStr = atob(cat64);
  let cat = new Uint8Array(catStr.length);
  for (let /** @type{number}*/ i = 0; i < catStr.length; ++i) cat[i] = catStr.charCodeAt(i);
  console.log(cat.length);
  let catIm = Decoder.decode(cat);
  const canvas = /** @type{!HTMLCanvasElement} */ (document.getElementById('canvas'));
  const ctx = /** @type{!CanvasRenderingContext2D} */ (canvas.getContext('2d'));
  ctx.putImageData(catIm, 0, 0);

  /** @param{string} text */
  function report(text) {
    let p = document.createElement('p');
    let t = document.createTextNode(text);
    p.appendChild(t);
    content.appendChild(p);
  }

  /**
   * @param{string} str
   * @return{number}
   */
  function chr(str) {
    return str.codePointAt(0);
  }

  function decodeInput() {
    let materialized = input.value;
    let dematerialized = Array.from(materialized).map(chr);
    let decoded = CjkDecoder.decode(dematerialized);
    report(decoded.join(', '));
  }

  input.addEventListener('input', decodeInput);
});
