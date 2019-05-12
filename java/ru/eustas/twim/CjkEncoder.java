package ru.eustas.twim;

/**
 * Byte array to CJK string encoder.
 */
final class CjkEncoder {
  // Collection of utilities. Should not be instantiated.
  CjkEncoder() {}

  static int[] encode(byte[] data) {
    int bufferLength = (5 * (((data.length * 8) + 40) / 41) + 1) / 2;
    int[] nibbles = new int[2 * bufferLength];
    int nibbleIdx = 0;
    int offset = 0;
    while (offset < data.length * 8) {
      long p = 0;
      int ix = offset >> 3;
      for (int i = 0; i < 6; ++i) {
        int idx = ix + 5 - i;
        int b = (idx < data.length) ? (data[idx] & 0xFF) : 0;
        p = (p << 8) | b;
      }
      p = (p >> (offset & 7)) & 0x1FFFFFFFFFFL;
      offset += 41;
      for (int i = 0; i < 5; ++i) {
        nibbles[nibbleIdx++] = (int) (p % 296L);
        p = p / 296L;
      }
    }
    int[] buffer = new int[bufferLength];
    for (int i = 0; i < nibbles.length; i += 2) {
      buffer[i / 2] = CjkTransform.ordinalToUnicode(nibbles[i] + 296 * nibbles[i + 1]);
    }
    int unicodeZero = CjkTransform.ordinalToUnicode(0);
    while (bufferLength > 0 && buffer[bufferLength - 1] == unicodeZero) {
      bufferLength--;
    }
    int[] result = new int[bufferLength];
    System.arraycopy(buffer, 0, result, 0, bufferLength);
    return result;
  }
}