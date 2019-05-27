package ru.eustas.twim;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class SinCosTest {
  @Test
  public void testSin() {
    long crc = Crc64.init();
    for (int i = 0; i < SinCos.MAX_ANGLE; ++i) {
      crc = Crc64.update(crc, (byte) (SinCos.SIN[i] & 0xFF));
    }
    assertEquals("9486473C3841E28F", Crc64.finish(crc));
  }

  @Test
  public void testCos() {
    long crc = Crc64.init();
    for (int i = 0; i < SinCos.MAX_ANGLE; ++i) {
      crc = Crc64.update(crc, (byte) (SinCos.COS[i] & 0xFF));
    }
    assertEquals("A32700985A177AE9", Crc64.finish(crc));
  }
}
