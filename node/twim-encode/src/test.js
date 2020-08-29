let createTwimEncoder = require('../index.js');
createTwimEncoder().then((e) => {
  let rgba = new Uint8Array(1600);
  // [12, 26, 38, 52, 64]
  let q = (v) => { let r = 0; while (v >= [4, 8, 12, 16, 20][r]) r++; return r; }
  for (let i = 0; i < 400; i++) {
    let x = q(i % 20);
    let y = q((i / 20)|0);
    rgba.set((0x23880 >> (x + 5 * y) & 1) ? [255, 255, 255] : [255], i * 4);
  }
  e.setTargetSize(23);
  e.setVariants([{partitionCode: 499, lineLimit: 2, colorOptions: [18]}]);
  let result = e.encodeImage({rgba: rgba, width: 20, height: 20});
  let ok = (result.mse < 1e-3) && (result.data.length <= 23);
  if (!ok) {
    console.log("MSE: " + result.mse);
    console.log("Data: " + Buffer.from(result.data).toString('base64'));
  }
  process.exit(ok ? 0 : 1);
});