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
    byte[] encoded = {-125, -119, -8, 39, 89, 113, 0, -128, -1, -1, -1, 62, 75, 106, -28, -75, 70,
        -53, -57, -52, 27, 8, 22};
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
