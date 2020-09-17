function createSampleImage() {
  let rgba = new Uint8Array(1600);
  for (let i = 0; i < 400; i++) {
    let x = (i % 20) >> 2;
    let y = (i / 20) >> 2;
    rgba.set((0x23880 >> (x + 5 * y) & 1) ? [255, 255, 255, 255] : [255, 0, 0, 255], i * 4);
  }
  return rgba;
}
function assertTrue(ok) {if (!ok) process.exit(1); }

let encoded = Uint8Array.from([131, 137, 248, 39, 89, 113, 0, 128, 255, 255, 255, 62, 75, 106, 228, 181, 70, 203, 199, 204, 27, 8, 22]);
let decodeTwim = require('../index.js');
let imageData = decodeTwim(encoded);
assertTrue(imageData.width == 20);
assertTrue(imageData.height == 20);
let rgba = createSampleImage();
for (let i = 0; i < 1600; ++i) assertTrue(rgba[i] == imageData.rgba[i]);
