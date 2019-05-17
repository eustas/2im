package ru.eustas.twim;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertArrayEquals;

@RunWith(JUnit4.class)
public class CjkTest {
  private void checkRoundtrip(byte[] bytes) {
    int[] codePoints = CjkEncoder.encode(bytes);
    byte[] decoded = CjkDecoder.decode(codePoints);
    assertArrayEquals(bytes, decoded);
  }

  @Test
  public void testCoverage() {
    new CjkDecoder();
    new CjkEncoder();
    new CjkTransform();
  }

  @Test
  public void testEmpty() {
    checkRoundtrip(new byte[0]);
  }

  @Test
  public void testOneByte() {
    checkRoundtrip(new byte[] {42});
  }

  @Test
  public void testZeroTruncation() {
    int[] codePoints = CjkEncoder.encode(new byte[] {42, 0, 0, 0, 0, 0, 43, 0, 0, 0});
    byte[] decoded = CjkDecoder.decode(codePoints);
    assertArrayEquals(new byte[] {42, 0, 0, 0, 0, 0, 43}, decoded);
  }

  @Test
  public void testRandom() {
    checkRoundtrip(new byte[] {(byte) 0x20, (byte) 0x4c, (byte) 0x75, (byte) 0x2f, (byte) 0xa1,
        (byte) 0xd6, (byte) 0xe3, (byte) 0xb4, (byte) 0x21, (byte) 0x1f, (byte) 0x43, (byte) 0x77,
        (byte) 0x1d, (byte) 0xc8, (byte) 0x4b, (byte) 0x7c, (byte) 0xfa, (byte) 0x6d, (byte) 0xf6,
        (byte) 0x6c, (byte) 0x8a, (byte) 0x78, (byte) 0x9c, (byte) 0x47, (byte) 0x60, (byte) 0xdd,
        (byte) 0xae, (byte) 0x90, (byte) 0xb8, (byte) 0xee, (byte) 0x38, (byte) 0xb5, (byte) 0x07,
        (byte) 0x9e, (byte) 0x2a, (byte) 0x3d, (byte) 0xb0, (byte) 0xc0, (byte) 0x4e, (byte) 0x69,
        (byte) 0xcf, (byte) 0x32});
  }
}
