goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('twim.Region');
goog.require('twim.SinCos');

function testHorizontalSplit() {
  let /** @type{!Int32Array} */ region = new Int32Array([0, 0, 4, 1]);
  let /** @type{number} */ angle = (twim.SinCos.MAX_ANGLE / 2) | 0;
  let /** @type{!twim.Region.DistanceRange} */ distanceRange = new twim.Region.DistanceRange();
  distanceRange.update(region, angle, twim.SinCos.SCALE);
  assertEquals(3, distanceRange.numLines);
  let /** @type{!Int32Array} */ left = new Int32Array(4);
  let /** @type{!Int32Array} */ right = new Int32Array(4);
  // 1/3
  twim.Region.splitLine(region, angle, distanceRange.distance(0), left, right);
  assertSameElements(new Int32Array([0, 1, 4, 1]), left);
  assertSameElements(new Int32Array([0, 0, 1, 1]), right);
  // 2/3
  twim.Region.splitLine(region, angle, distanceRange.distance(1), left, right);
  assertSameElements(new Int32Array([0, 2, 4, 1]), left);
  assertSameElements(new Int32Array([0, 0, 2, 1]), right);
  // 3/3
  twim.Region.splitLine(region, angle, distanceRange.distance(2), left, right);
  assertSameElements(new Int32Array([0, 3, 4, 1]), left);
  assertSameElements(new Int32Array([0, 0, 3, 1]), right);
}

function testVerticalSplit() {
  let /** @type{!Int32Array} */ region = new Int32Array([0, 0, 1, 1, 0, 1, 2, 0, 1, 3, 0, 1, 4]);
  let /** @type{number} */ angle = 0;
  let /** @type{!twim.Region.DistanceRange} */ distanceRange = new twim.Region.DistanceRange();
  distanceRange.update(region, angle, twim.SinCos.SCALE);
  assertEquals(3, distanceRange.numLines);
  {
    // 1/3
    let /** @type{!Int32Array} */ left = new Int32Array(10);
    let /** @type{!Int32Array} */ right = new Int32Array(4);
    twim.Region.splitLine(region, angle, distanceRange.distance(0), left, right);
    assertSameElements(new Int32Array([1, 0, 1, 2, 0, 1, 3, 0, 1, 3]), left);
    assertSameElements(new Int32Array([0, 0, 1, 1]), right);
  }
  {
    // 2/3
    let /** @type{!Int32Array} */ left = new Int32Array(7);
    let /** @type{!Int32Array} */ right = new Int32Array(7);
    twim.Region.splitLine(region, angle, distanceRange.distance(1), left, right);
    assertSameElements(new Int32Array([2, 0, 1, 3, 0, 1, 2]), left);
    assertSameElements(new Int32Array([0, 0, 1, 1, 0, 1, 2]), right);
  }
  {
    let /** @type{!Int32Array} */ left = new Int32Array(4);
    let /** @type{!Int32Array} */ right = new Int32Array(10);
    twim.Region.splitLine(region, angle, distanceRange.distance(2), left, right);
    assertSameElements(new Int32Array([3, 0, 1, 1]), left);
    assertSameElements(new Int32Array([0, 0, 1, 1, 0, 1, 2, 0, 1, 3]), right);
    // 3/3
  }
}
