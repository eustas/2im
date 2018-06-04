package ru.eustas.twim;

import java.io.ByteArrayOutputStream;

/**
 * CJK string to byte array decoder.
 */
final class CjkDecoder {

  // Collection of utilities. Should not be instantiated.
  CjkDecoder() {}

  static byte[] decode(int[] data) {
    int[] nibbles = new int[data.length * 2];
    for (int i = 0; i < data.length; ++i) {
      int ord = CjkTransform.unicodeToOrdinal(data[i]);
      nibbles[2 * i] = ord % 296;
      nibbles[2 * i + 1] = ord / 296;
    }
    ByteArrayOutputStream result = new ByteArrayOutputStream();
    int ix = 0;
    long acc = 0;
    int bits = 0;
    int zeroCount = 0;
    while (ix < nibbles.length || acc != 0) {
      if (bits < 8) {
        long p = 0;
        for (int j = 0; j < 5; ++j) {
          int idx = ix + 4 - j;
          int nibble = (idx < nibbles.length) ? nibbles[idx] : 0;
          p = (p * 296) + nibble;
        }
        ix += 5;
        acc += p << bits;
        bits += 41;
      }
      byte b = (byte)acc;
      if (b != 0) {
        while (zeroCount != 0) {
          result.write(0);
          zeroCount--;
        }
        result.write(b);
      } else {
        zeroCount++;
      }
      acc = acc >> 8;
      bits -= 8;
    }
    return result.toByteArray();
  }
}