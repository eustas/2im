package ru.eustas.twim;

import org.junit.Test;

import static org.junit.Assert.*;

public class Crc64Test {

  @Test
  public void testCrc64() {
    long crc = -1;
    for (int i = 0; i < 10; ++i) crc = Crc64.update(crc, (byte)(97 + i));
    crc = crc ^ -1;
    assertEquals(0x32093A2ECD5773F4L, crc);
  }
}