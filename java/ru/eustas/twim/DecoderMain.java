package ru.eustas.twim;

import javax.imageio.ImageIO;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;

public class DecoderMain {
  public static void main(String... args) throws IOException {
    for (String arg : args) {
      byte[] data = Files.readAllBytes(FileSystems.getDefault().getPath(arg));
      ImageIO.write(Decoder.decode(data, 64, 64), "png", new File(arg + ".png"));
    }
  }
}
