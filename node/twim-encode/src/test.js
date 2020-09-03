function createSampleImage() {
  let rgba = new Uint8Array(1600);
  for (let i = 0; i < 400; i++) {
    let x = (i % 20) >> 2;
    let y = (i / 20) >> 2;
    rgba.set((0x23880 >> (x + 5 * y) & 1) ? [255, 255, 255] : [255], i * 4);
  }
  return rgba;
}
function assertTrue(ok) {if (!ok) process.exit(1); }

let createTwimEncoder = require('../index.js');
// NB: it is better to reuse encoder instance!
createTwimEncoder().then((e) => {
// >>> how to use
  e.setTargetSize(23);
  e.setVariants([{partitionCode: 499, lineLimit: 2, colorOptions: [18]}]);
  let result = e.encodeImage({rgba: createSampleImage(), width: 20, height: 20});
// <<< how to use
  assertTrue((result.mse < 1e-3) && (result.data.length <= 23));
});