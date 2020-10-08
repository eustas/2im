<p align="center"><img src="https://eustas.github.io/2im/cat.gif" /></p>

### Introduction

2im (twim) is an image format designed for extremely-low BPP (bits-per-pixels) range. Name of format hints that a single Twitter message (140 CJK characters, worth at least 287 bytes of payload) is enough to transfer an image.

Unlike [SQIP](https://github.com/axe312ger/sqip) ([primitive](https://github.com/fogleman/primitive)), output is not directly understood by browser. A tiny (1089 bytes, if compressed) JavaScript decoder is used to rasterize input to [ImageData](https://developer.mozilla.org/en-US/docs/Web/API/ImageData).

See also: [interactive demo page](https://eustas.github.io/2im/) and [twim vs SQIP comparison](https://eustas.github.io/2im/twim-vs-sqip.html).

### Coming soon
 - [ ] add build / use manual
 - [x] better CLI (set number of therads, "fast" encoding mode)
 - [x] publish decoder as NPM module
 - [x] publish encoder module
 - [ ] synchronize Java and C++ implementations
