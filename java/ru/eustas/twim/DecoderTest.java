package ru.eustas.twim;

import static org.junit.Assert.assertEquals;

import java.awt.image.BufferedImage;
import org.junit.Test;

public class DecoderTest {

  static int getCrossPixel(int x, int y) {
    return (0x23880 >> ((x >> 2) + 5 * (y >> 2)) & 1) == 1 ? 0xFFFFFFFF : 0xFFFF0000;
  }

  @Test
  public void testDecoder() {
    byte[] encoded = {50, -103, -66, 110, -65, 119, 0, -128, -1, -1, -1, 110, 127, -46, 48, -77, 28, 6, -19, 111, -22, 121, 77};
    BufferedImage decoded = Decoder.decode(encoded);
    assertEquals(20, decoded.getWidth());
    assertEquals(20, decoded.getHeight());
    for (int y = 0; y < 20; ++y) {
      for (int x = 0; x < 20; ++x) {
        assertEquals(getCrossPixel(x, y), decoded.getRGB(x, y));
      }
    }
  }
}
