package ru.eustas.twim;

import static org.junit.Assert.*;

import org.junit.Test;

public class Crc64Test {
  @Test
  public void testCrc64() {
    long crc = Crc64.init();
    for (int i = 0; i < 10; ++i) crc = Crc64.update(crc, (byte) (97 + i));
    assertEquals("32093A2ECD5773F4", Crc64.finish(crc));
  }
}