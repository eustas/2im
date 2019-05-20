package ru.eustas.twim;

import javax.imageio.ImageIO;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;

public class EncoderMain {
  public static void main(String... args) throws IOException {
    for (String arg : args) {
      byte[] data = Encoder.encode(ImageIO.read(new File(arg)));
      Files.write(FileSystems.getDefault().getPath(arg + ".2im"), data);
    }
  }
}
