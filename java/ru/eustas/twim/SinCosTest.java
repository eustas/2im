package ru.eustas.twim;

import org.junit.Test;

import static org.junit.Assert.*;

public class SinCosTest {

  @Test
  public void testSin() {
    long crc = Crc64.init();
    for (int i = 0; i < SinCos.MAX_ANGLE; ++i) crc = Crc64.update(crc, (byte)(SinCos.SIN[i] & 0xFF));
    assertEquals("F22E8BD6F4993380", Crc64.finish(crc));
  }

  @Test
  public void testCos() {
    long crc = Crc64.init();
    for (int i = 0; i < SinCos.MAX_ANGLE; ++i) crc = Crc64.update(crc, (byte)(SinCos.COS[i] & 0xFF));
    assertEquals("D18BB3517EE71BBB", Crc64.finish(crc));
  }
}