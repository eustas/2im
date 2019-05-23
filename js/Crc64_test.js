import {assertEquals} from 'goog:goog.testing.asserts';
import 'goog:goog.testing.jsunit';
import * as Crc64 from './Crc64.js';

function testCrc64() {
  let /** @type{!Uint32Array} */ crc = Crc64.init();
  for (let /** @type{number} */ i = 0; i < 10; ++i) crc = Crc64.update(crc, 97 + i);
  assertEquals('32093A2ECD5773F4', Crc64.finish(crc));
}
