package ru.eustas.twim;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.fail;

@RunWith(JUnit4.class)
public class UtfTest {

  private void checkRoundtrip(int[] codePoints) {
    byte[] encoded = UtfEncoder.encode(codePoints);
    int[] decoded = UtfDecoder.decode(encoded);
    assertArrayEquals(codePoints, decoded);
  }

  @Test
  public void testCoverage() {
    new UtfDecoder();
    new UtfEncoder();
  }

  @Test
  public void testEmpty() {
    checkRoundtrip(new int[0]);
  }

  @Test
  public void testAllRanges() {
    checkRoundtrip(new int[] {42, 0x90, 0x900, 0x1100});
  }

  @Test
  public void testCodePointOutOfRange() {
    try {
      UtfEncoder.encode(new int[] {1 << 21});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testUnexpectedContinuation() {
    try {
      UtfDecoder.decode(new byte[] {(byte) 0x81});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testUnexpectedLength() {
    try {
      UtfDecoder.decode(new byte[] {(byte) 0xF8});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testUnfinishedRune() {
    try {
      UtfDecoder.decode(new byte[] {(byte) 0xC0});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }

  @Test
  public void testBrokenRune() {
    try {
      UtfDecoder.decode(new byte[] {(byte) 0xC0, 0});
    } catch (IllegalArgumentException ex) {
      return;
    }
    fail();
  }
}