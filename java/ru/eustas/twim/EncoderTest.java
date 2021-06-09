package ru.eustas.twim;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.awt.image.BufferedImage;

import org.junit.Test;

public class EncoderTest {

  static int getCrossPixel(int x, int y) {
    return (0x23880 >> ((x >> 2) + 5 * (y >> 2)) & 1) == 1 ? 0xFFFFFFFF : 0xFFFF0000;
  }

  static BufferedImage makeCross() {
    BufferedImage result = new BufferedImage(20, 20, BufferedImage.TYPE_INT_ARGB);
    for (int y = 0; y < 20; ++y) {
      for (int x = 0; x < 20; ++x) {
        result.setRGB(x, y, getCrossPixel(x, y));
      }
    }
    return result;
  }

  // TODO(eustas): move to CodecParamsTest?
  @Test
  public void testImageTax() {
    XRangeEncoder dst = new XRangeEncoder();
    CodecParams cp = new CodecParams(8, 8);
    cp.write(dst);
    assertEquals(dst.estimateEntropy(), CodecParams.calculateImageTax(8, 8), 1e-8f);
  }

  @Test
  public void testEncoder() {
    BufferedImage original = makeCross();
    Encoder.Params params = new Encoder.Params();
    params.targetSize = 23;
    Encoder.Variant variant = new Encoder.Variant();
    variant.partitionCode = 499;
    variant.lineLimit = 2;
    //variant.colorOptions = 1 << 18;
    variant.colorOptions = ~0L;
    params.variants.add(variant);
    Encoder.Result result = Encoder.encode(original, params);
    //assertTrue(result.mse < 1e-3);
    //assertTrue(result.data.length <= 23);
  }
}
