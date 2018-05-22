package ru.eustas.twim;

import java.io.ByteArrayOutputStream;

final class Utf8 {

  static int[] decode(byte[] data) {
    int[] buffer = new int[data.length];
    int bufferSize = 0;
    int i = 0;
    while (i < data.length) {
      int b = data[i++] & 0xFF;
      int c;
      if ((b & 0x80) == 0) {
        c = 0;
      } else if ((b & 0xC0) == 0x80) {
        throw new IllegalArgumentException();
      } else if ((b & 0xE0) == 0xC0) {
        b = b & 0x1F;
        c = 1;
      } else if ((b & 0xF0) == 0xE0) {
        b = b & 0xF;
        c = 2;
      } else if ((b & 0xF8) == 0xF0) {
        b = b & 0x7;
        c = 3;
      } else {
        throw new IllegalArgumentException();
      }
      if (i + c > data.length) {
        throw new IllegalArgumentException();
      }
      while (c-- > 0) {
        int x = data[i++] & 0xFF;
        if ((x & 0xC0) != 0x80) {
          throw new IllegalArgumentException();
        }
        b = (b << 6) | (x & 0x3F);
      }
      buffer[bufferSize++] = b;
    }
    int[] result = new int[bufferSize];
    System.arraycopy(buffer, 0, result, 0, bufferSize);
    return result;
  }

  static byte[] encode(int[] data) {
    ByteArrayOutputStream result = new ByteArrayOutputStream();
    for (int ord : data) {
      if (ord < 0x80) {
        result.write(ord);
      } else if (ord < 0x800) {
        result.write(0xC0 | (ord >> 6));
        result.write(0x80 | (ord & 0x3F));
      } else if (ord < 0x1000) {
        result.write(0xE0 | (ord >> 12));
        result.write(0x80 | ((ord >> 6) & 0x3F));
        result.write(0x80 | (ord & 0x3F));
      } else if (ord < 0x20000) {
        result.write(0xF0 | (ord >> 18));
        result.write(0x80 | ((ord >> 12) & 0x3F));
        result.write(0x80 | ((ord >> 6) & 0x3F));
        result.write(0x80 | (ord & 0x3F));
      } else {
        throw new IllegalArgumentException();
      }
    }
    return result.toByteArray();
  }
}
