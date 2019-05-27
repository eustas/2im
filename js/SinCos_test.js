import {assertEquals} from 'goog:goog.testing.asserts';
import 'goog:goog.testing.jsunit';
import * as Crc64 from './Crc64.js';
import * as SinCos from './SinCos.js';

function testSin() {
  let /** @type{!Uint32Array} */ crc = Crc64.init();
  for (let /** @type{number} */ i = 0; i < SinCos.MAX_ANGLE; ++i) {
    console.log(SinCos.SIN[i]);
    crc = Crc64.update(crc, SinCos.SIN[i] & 0xFF);
  }
  assertEquals('9486473C3841E28F', Crc64.finish(crc));
}

function testCos() {
  let /** @type{!Uint32Array} */ crc = Crc64.init();
  for (let /** @type{number} */ i = 0; i < SinCos.MAX_ANGLE; ++i) {
    crc = Crc64.update(crc, SinCos.COS[i] & 0xFF);
  }
  assertEquals('A32700985A177AE9', Crc64.finish(crc));
}
