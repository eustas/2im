package ru.eustas.twim;

import org.junit.Test;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;

public class DecoderTest {

  /*@Test
  public void testEncoder() throws IOException {
    String arg = "cat.png.2im";
    byte[] data = Files.readAllBytes(FileSystems.getDefault().getPath(arg));
    BufferedImage decoded = Decoder.decode(data);
    ImageIO.write(decoded, "png", new File(arg + ".png"));
  }*/
}
