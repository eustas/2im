goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('twim.Crc64');
goog.require('twim.SinCos');

function testSin() {
  let crc = twim.Crc64.init();
  for (let i = 0; i < twim.SinCos.MAX_ANGLE; ++i) {
    crc = twim.Crc64.update(crc, twim.SinCos.SIN[i] & 0xFF);
  }
  assertEquals('F22E8BD6F4993380', twim.Crc64.finish(crc));
}

function testCos() {
  let crc = twim.Crc64.init();
  for (let i = 0; i < twim.SinCos.MAX_ANGLE; ++i) {
    crc = twim.Crc64.update(crc, twim.SinCos.COS[i] & 0xFF);
  }
  assertEquals('D18BB3517EE71BBB', twim.Crc64.finish(crc));
}
