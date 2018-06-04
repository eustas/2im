package ru.eustas.twim;

final class RangeCode {
  static final int NUM_NIBBLES = 6;
  static final int NIBBLE_BITS = 8;
  static final int NIBBLE_MASK = (1 << NIBBLE_BITS) - 1;
  private static final int VALUE_BITS = NUM_NIBBLES * NIBBLE_BITS;
  static final long VALUE_MASK = (1L << VALUE_BITS) - 1L;
  static final int HEAD_NIBBLE_SHIFT = VALUE_BITS - NIBBLE_BITS;
  static final long HEAD_START = 1L << HEAD_NIBBLE_SHIFT;
  private static final int RANGE_LIMIT_BITS = HEAD_NIBBLE_SHIFT - NIBBLE_BITS;
  static final long RANGE_LIMIT_MASK = (1L << RANGE_LIMIT_BITS) - 1L;
}
