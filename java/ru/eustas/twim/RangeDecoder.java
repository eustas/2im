package ru.eustas.twim;

import static ru.eustas.twim.RangeCode.HEAD_START;
import static ru.eustas.twim.RangeCode.NIBBLE_BITS;
import static ru.eustas.twim.RangeCode.NIBBLE_MASK;
import static ru.eustas.twim.RangeCode.NUM_NIBBLES;
import static ru.eustas.twim.RangeCode.RANGE_LIMIT_MASK;
import static ru.eustas.twim.RangeCode.VALUE_MASK;

final class RangeDecoder {
  private long low;
  private long range = VALUE_MASK;
  private long code;
  private final byte[] data;
  private int offset;

  RangeDecoder(byte[] data) {
    this.data = data;
    long code = 0;
    for (int i = 0; i < NUM_NIBBLES; ++i) {
      code = (code << NIBBLE_BITS) | readNibble();
    }
    this.code = code;
  }

  private int readNibble() {
    return (offset < data.length) ? (data[offset++] & NIBBLE_MASK) : 0;
  }

  final void removeRange(int bottom, int top) {
    low += bottom * range;
    range *= top - bottom;
    while (true) {
      if ((low ^ (low + range - 1)) >= HEAD_START) {
        if (range > RANGE_LIMIT_MASK) {
          break;
        }
        range = -low & RANGE_LIMIT_MASK;
      }
      code = ((code << NIBBLE_BITS) & VALUE_MASK) | readNibble();
      range = (range << NIBBLE_BITS) & VALUE_MASK;
      low = (low << NIBBLE_BITS) & VALUE_MASK;
    }
  }

  final int currentCount(int totalRange) {
    range /= totalRange;
    return (int) ((code - low) / range);
  }
}
