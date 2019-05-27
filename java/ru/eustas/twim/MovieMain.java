package ru.eustas.twim;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.util.List;

public class MovieMain {
  public static void main(String... args) throws IOException, InterruptedException {
    String arg = args[0];
    BufferedImage image = ImageIO.read(new File(arg));
    int width = image.getWidth();
    int height = image.getHeight();
    Encoder.Cache cache = new Encoder.Cache(image.getRGB(0, 0, width, height, null, 0, width), width);
    CodecParams cp = new CodecParams(width, height);
    cp.setPartitionCode(222);
    cp.setColorCode(6);
    List<Encoder.Fragment> partition = Encoder.buildPartition(512, 3, cp, cache);

    for (int targetSize = 7; targetSize < 500; ++targetSize) {
      String suffix = String.format(".%03d", targetSize);
      byte[] data = Encoder.encode(targetSize, partition, cp);
      Files.write(FileSystems.getDefault().getPath(arg + suffix + ".2im"), data);
      ImageIO.write(Decoder.decode(data, width, height), "png", new File(arg + suffix + ".png"));
    }
  }
}
