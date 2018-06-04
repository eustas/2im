package ru.eustas.twim;

import java.io.ByteArrayOutputStream;

import static ru.eustas.twim.RangeCode.HEAD_NIBBLE_SHIFT;
import static ru.eustas.twim.RangeCode.HEAD_START;
import static ru.eustas.twim.RangeCode.NIBBLE_BITS;
import static ru.eustas.twim.RangeCode.NUM_NIBBLES;
import static ru.eustas.twim.RangeCode.RANGE_LIMIT_MASK;
import static ru.eustas.twim.RangeCode.VALUE_MASK;

final class RangeEncoder {
  private long low;
  private long range = VALUE_MASK;
  private final ByteArrayOutputStream data = new ByteArrayOutputStream();

  final void encodeRange(int bottom, int top, int totalRange) {
    range /= totalRange;
    low += bottom * range;
    range *= top - bottom;
    while (true) {
      if ((low ^ (low + range - 1)) >= HEAD_START) {
        if (range > RANGE_LIMIT_MASK) {
          break;
        }
        range = -low & RANGE_LIMIT_MASK;
      }
      data.write((byte) (low >> HEAD_NIBBLE_SHIFT));
      range = (range << NIBBLE_BITS) & VALUE_MASK;
      low = (low << NIBBLE_BITS) & VALUE_MASK;
    }
  }

  final byte[] finish() {
    for (int i = 0; i < NUM_NIBBLES; ++i) {
      data.write((byte)(low >> HEAD_NIBBLE_SHIFT));
      low = (low << NIBBLE_BITS) & VALUE_MASK;
    }
    return data.toByteArray();
  }
}
