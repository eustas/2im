package ru.eustas.twim;

import static ru.eustas.twim.XRangeCode.BITS;
import static ru.eustas.twim.XRangeCode.MASK;
import static ru.eustas.twim.XRangeCode.MIN;
import static ru.eustas.twim.XRangeCode.SPACE;

final class XRangeDecoder {
  private int state = MIN;
  private int pos = 0;
  private final byte[] data;

  XRangeDecoder(byte[] data) {
    this.data = data;
    for (int i = 0; i < BITS; ++i) {
      state |= nextBit() << i;
    }
  }

  static int readNumber(XRangeDecoder src, int max) {
    if (max == 0) throw new IllegalStateException("max must be not less than 1");
    if (max == 1) return 0;
    int state = src.state;
    int offset = state & MASK;
    int result = (offset * max + max - 1) / SPACE;
    int low = result * SPACE;
    int base = low / max;
    int freq = (low + SPACE) / max - base;
    state = freq * (state / SPACE) + offset - base;
    while (state < MIN) state = (state << 1) | src.nextBit();
    src.state = state;
    return result;
  }

  private int nextBit() {
    int offset = pos >> 3;
    if (offset >= data.length) return 0;
    return (data[offset] >> (pos++ & 7)) & 1;
  }
}
