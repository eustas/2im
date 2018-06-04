package ru.eustas.twim;

import java.io.ByteArrayOutputStream;

final class UtfEncoder {

  // Collection of utilities. Should not be instantiated.
  UtfEncoder() {}

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
