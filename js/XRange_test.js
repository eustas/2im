import {assertEquals} from 'goog:goog.testing.asserts';
import 'goog:goog.testing.jsunit';
import * as XRangeDecoder from "./XRangeDecoder.js";

function testOptimizer() {
    let length = 12;
    let /** @type{!Uint8Array} */ expected = new Uint8Array([-78, -15, 81, -45, -48, -46, -47, 51, 48, 50, 49, -77]);
    XRangeDecoder.init(expected);
    assertEquals(13, XRangeDecoder.readNumber(63));
    for (let /** @type{number} */ i = 0; i < length - 1; ++i) {
        assertEquals(i + 42, XRangeDecoder.readNumber(256));
    }
}