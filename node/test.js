const fs = require('fs');
const twim = require('./twim');

encoded = twim.encode({data: new Uint8ClampedArray(400), width: 10, height: 10});

fs.writeFile('img.2im', Buffer.from(encoded), (err) => {
  if (err) throw err;
  console.log('The file has been saved!');
});
