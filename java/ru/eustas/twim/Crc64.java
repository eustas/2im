package ru.eustas.twim;

public class Crc64 {
  private static final long CRC_64_POLY = (0xC96C5795L << 32) | 0xD7870F42L;

  /**
   * Roll CRC64 calculation.
   *
   * <p> {@code CRC64(data) = finish(update((... update(init(), firstByte), ...), lastByte));}
   * <p> This simple and reliable checksum is chosen to make is easy to calculate the same value
   * across the variety of languages (C++, Java, Go, ...).
   */
  public static long update(long crc, byte next) {
    long c = (crc ^ ((long)(next & 0xFF))) & 0xFF;
    for (int i = 0; i < 8; ++i) {
      boolean b = ((c & 1) == 1);
      long d = c >>> 1;
      c = b ? (CRC_64_POLY ^ d) : d;
    }
    return c ^ (crc >>> 8);
  }

  public static long init() {
    return -1;
  }

  public static String finish(long crc) {
    crc = ~crc;
    char[] buffer = new char[16];
    for (int i = 15; i >= 0; --i) {
      buffer[i] = "0123456789ABCDEF".charAt((int)crc & 0xF);
      crc >>>= 4;
    }
    return new String(buffer);
  }
}
