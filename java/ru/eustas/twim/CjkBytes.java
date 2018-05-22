package ru.eustas.twim;

import java.io.ByteArrayOutputStream;

/**
 * Bytes-to-CJK codec.
 *
 * NB: CJK does not fit "Basic Multilingual Plane", i.e. can not be represented with "char".
 * To convert CJK to UTF-8 text and back use {@link Utf8}.
 */
final class CjkBytes {

  /* Pairs of values representing first and last scalars in CJK ranges. */
  private static final int[] CJK = new int[] {
    0x04E00, 0x09FFF, 0x03400, 0x04DBF, 0x20000, 0x2A6DF, 0x2A700,
    0x2B73F, 0x2B740, 0x2B81F, 0x2B820, 0x2CEAF, 0x2CEB0, 0x2EBEF
  };

  static byte[] decode(int[] data) {
    int[] nibbles = new int[data.length * 2];
    for (int i = 0; i < data.length; ++i) {
      int chr = data[i];
      int ord = 0;
      for (int j = 0; j < CJK.length; j += 2) {
        int size = CJK[j + 1] - CJK[j] + 1;
        if (chr >= CJK[j] && chr <= CJK[j + 1]) {
          ord += chr - CJK[j];
          break;
        }
        ord += size;
      }
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
        acc += p * (1 << bits);
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
        nibbles[nibbleIdx++] = (int)(p % 296L);
        p = p / 296L;
      }
    }
    int[] buffer = new int[bufferLength];
    for (int i = 0; i < nibbles.length; i += 2) {
      int ord = nibbles[i] + 296 * nibbles[i + 1];
      int chr = 0;
      for (int j = 0; j < CJK.length; j += 2) {
        int size = CJK[j + 1] - CJK[j] + 1;
        if (ord < size) {
          chr = ord + CJK[j];
          break;
        }
        ord -= size;
      }
      buffer[i/2] = chr;
    }
    while (bufferLength > 0 && buffer[bufferLength - 1] == CJK[0]) {
      bufferLength--;
    }
    int[] result = new int[bufferLength];
    System.arraycopy(buffer, 0, result, 0, bufferLength);
    return result;
  }
}