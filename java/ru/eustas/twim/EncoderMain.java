package ru.eustas.twim;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;

public class EncoderMain {
  public static void main(String... args) throws IOException, InterruptedException {
    for (String arg : args) {
      BufferedImage image = ImageIO.read(new File(arg));
      byte[] data = Encoder.encode(image, 48);
      Files.write(FileSystems.getDefault().getPath(arg + ".2im"), data);
    }
  }
}
