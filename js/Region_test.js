import {assertEquals, assertSameElements} from 'goog:goog.testing.asserts';
import 'goog:goog.testing.jsunit';
import {CodecParams} from './CodecParams.js';
import * as Region from './Region.js';
import * as SinCos from './SinCos.js';

const DistanceRange = Region.DistanceRange;

function testHorizontalSplit() {
  let /** @type{!CodecParams} */ cp = new CodecParams(4,4);
  let /** @type{!Int32Array} */ region = new Int32Array([0, 0, 4, 1]);
  let /** @type{number} */ angle = (SinCos.MAX_ANGLE / 2) | 0;
  let /** @type{!DistanceRange} */ distanceRange = new Region.DistanceRange();
  distanceRange.update(region, angle, cp);
  assertEquals(3, distanceRange.numLines);
  let /** @type{!Int32Array} */ left = new Int32Array(4);
  let /** @type{!Int32Array} */ right = new Int32Array(4);
  // 1/3
  Region.splitLine(region, angle, distanceRange.distance(0), left, right);
  assertSameElements(new Int32Array([0, 1, 4, 1]), left);
  assertSameElements(new Int32Array([0, 0, 1, 1]), right);
  // 2/3
  Region.splitLine(region, angle, distanceRange.distance(1), left, right);
  assertSameElements(new Int32Array([0, 2, 4, 1]), left);
  assertSameElements(new Int32Array([0, 0, 2, 1]), right);
  // 3/3
  Region.splitLine(region, angle, distanceRange.distance(2), left, right);
  assertSameElements(new Int32Array([0, 3, 4, 1]), left);
  assertSameElements(new Int32Array([0, 0, 3, 1]), right);
}

function testVerticalSplit() {
  let /** @type{!CodecParams} */ cp = new CodecParams(4,4);
  let /** @type{!Int32Array} */ region = new Int32Array([0, 0, 1, 1, 0, 1, 2, 0, 1, 3, 0, 1, 4]);
  let /** @type{number} */ angle = 0;
  let /** @type{!DistanceRange} */ distanceRange = new Region.DistanceRange();
  distanceRange.update(region, angle, cp);
  assertEquals(3, distanceRange.numLines);
  {
    // 1/3
    let /** @type{!Int32Array} */ left = new Int32Array(10);
    let /** @type{!Int32Array} */ right = new Int32Array(4);
    Region.splitLine(region, angle, distanceRange.distance(0), left, right);
    assertSameElements(new Int32Array([1, 0, 1, 2, 0, 1, 3, 0, 1, 3]), left);
    assertSameElements(new Int32Array([0, 0, 1, 1]), right);
  }
  {
    // 2/3
    let /** @type{!Int32Array} */ left = new Int32Array(7);
    let /** @type{!Int32Array} */ right = new Int32Array(7);
    Region.splitLine(region, angle, distanceRange.distance(1), left, right);
    assertSameElements(new Int32Array([2, 0, 1, 3, 0, 1, 2]), left);
    assertSameElements(new Int32Array([0, 0, 1, 1, 0, 1, 2]), right);
  }
  {
    // 3/3
    let /** @type{!Int32Array} */ left = new Int32Array(4);
    let /** @type{!Int32Array} */ right = new Int32Array(10);
    Region.splitLine(region, angle, distanceRange.distance(2), left, right);
    assertSameElements(new Int32Array([3, 0, 1, 1]), left);
    assertSameElements(new Int32Array([0, 0, 1, 1, 0, 1, 2, 0, 1, 3]), right);
  }
}
