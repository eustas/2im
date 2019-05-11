goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('twim.Crc64');

function testCrc64() {
  let crc = twim.Crc64.init();
  for (let i = 0; i < 10; ++i) twim.Crc64.update(crc, 97 + i);
  assertEquals("32093A2ECD5773F4", twim.Crc64.finish(crc));
}
