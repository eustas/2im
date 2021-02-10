package ru.eustas.twim;

final class XRangeCode {
  static final int SPACE = 1 << 11;
  static final int MASK = SPACE - 1;
  static final int BITS = 16;
  static final int MIN = 1 << BITS;
  static final int MAX = 2 * MIN;
}
