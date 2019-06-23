package ru.eustas.twim;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;

public class Main {
  enum Mode {
    DECODE,
    ENCODE,
    ENCODE_DECODE,
  }

  public static void main(String... args) throws IOException, InterruptedException {
    int target = 200;
    Mode mode = Mode.ENCODE;
    boolean appendSize = false;
    for (String arg : args) {
      if (arg.startsWith("-t")) {
        target = Integer.parseInt(arg.substring(2));
        continue;
      } else if (arg.equals("-d")) {
        mode = Mode.DECODE;
        continue;
      } else if (arg.equals("-r")) {
        mode = Mode.ENCODE_DECODE;
        continue;
      } else if (arg.equals("-e")) {
        mode = Mode.ENCODE;
        continue;
      } else if (arg.equals("-s")) {
        appendSize = true;
        continue;
      }

      String path = arg;

      if (mode == Mode.ENCODE || mode == Mode.ENCODE_DECODE) {
        BufferedImage image = ImageIO.read(new File(path));
        byte[] data = Encoder.encode(image, target);
        if (appendSize) {
          path = path + "." + data.length;
        }
        path = path + ".2im";
        Files.write(FileSystems.getDefault().getPath(path), data);
      }

      if (mode == Mode.DECODE || mode == Mode.ENCODE_DECODE) {
        byte[] data = Files.readAllBytes(FileSystems.getDefault().getPath(path));
        path = path + ".png";
        ImageIO.write(Decoder.decode(data), "png", new File(path));
      }
    }
  }
}
