package ru.eustas.twim;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.fail;

@RunWith(JUnit4.class)
public class Utf8Test {

  private void checkRoundtrip(int[] codePoints) {
    byte[] encoded = Utf8.encode(codePoints);
    int[] decoded = Utf8.decode(encoded);
    assertArrayEquals(codePoints, decoded);
  }

  @Test
  public void testEmpty() {
    checkRoundtrip(new int[0]);
  }

  @Test
  public void testAllRanges() {
    checkRoundtrip(new int[] {0x80 - 42, 0x80 + 42, 0x10000 - 1961, 0x10000 + 1961});
  }

  @Test
  public void testCodePointOutOfRange() {
    try {
      Utf8.encode(new int[] {1 << 21});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testUnexpectedContinuation() {
    try {
      Utf8.decode(new byte[] {(byte) 0x81});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testUnexpectedLength() {
    try {
      Utf8.decode(new byte[] {(byte) 0xF8});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testUnfinishedRune() {
    try {
      Utf8.decode(new byte[] {(byte) 0xC0});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testBrokenRune() {
    try {
      Utf8.decode(new byte[] {(byte) 0xC0, 0});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }
}