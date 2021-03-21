package ru.eustas.twim;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertArrayEquals;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class XRangeTest {

  static int rng(int[] state) {
    long next = state[0];
    next = (next * 16807) % 0x7FFFFFFF;
    state[0] = (int) next;
    return state[0];
  }

  void checkRandom(int numRounds, int numItems, int seed) {
    int[] rngState = {seed};
    int[] items = new int[numItems * 2];
    for (int r = 0; r < numRounds; ++r) {
      XRangeEncoder encoder = new XRangeEncoder();
      for (int i = 0; i < numItems * 2; i += 2) {
        int total = 1 + rng(rngState) % 42;
        int val = rng(rngState) % total;
        XRangeEncoder.writeNumber(encoder, total, val);
        items[i] = val;
        items[i + 1] = total;
      }
      byte[] data = encoder.finish();

      XRangeDecoder decoder = new XRangeDecoder(data);
      for (int i = 0; i < numItems * 2; i += 2) {
        int val = XRangeDecoder.readNumber(decoder, items[i + 1]);
        assertEquals(items[i], val);
      }
    }
  }

  @Test
  public void testRandom10() {
    checkRandom(1000, 10, 42);
  }

  @Test
  public void testRandom30() {
    checkRandom(1000, 30, 44);
  }

  @Test
  public void testRandom50() {
    checkRandom(1000, 50, 46);
  }

  @Test
  public void testRandom70() {
    checkRandom(1000, 70, 48);
  }

  @Test
  public void testRandom90() {
    checkRandom(1000, 90, 50);
  }

  @Test
  public void testRandom10000000() {
    checkRandom(1, 10000000, 51);
  }

  @Test
  public void testOptimizer() {
    XRangeEncoder encoder = new XRangeEncoder();
    int l = 12;
    // 2 bits of overhead is bearable; amount depends on luck.
    XRangeEncoder.writeNumber(encoder, 63, 13);
    for (int i = 0; i < l - 1; ++i) {
      XRangeEncoder.writeNumber(encoder, 256, i + 42);
    }
    byte[] data = encoder.finish();

    byte[] expected = {-78, -15, 81, -45, -48, -46, -47, 51, 48, 50, 49, -77};

    assertArrayEquals(data, expected);

    XRangeDecoder decoder = new XRangeDecoder(data);
    assertEquals(13, XRangeDecoder.readNumber(decoder, 63));
    for (int i = 0; i < l - 1; ++i) {
      assertEquals(i + 42, XRangeDecoder.readNumber(decoder, 256));
    }
  }
}
