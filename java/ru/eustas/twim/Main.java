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

  public static void main(String... args) throws IOException {
    Mode mode = Mode.ENCODE;
    Encoder.Params params = new Encoder.Params();
    for (int lineLimit = 0; lineLimit < CodecParams.MAX_LINE_LIMIT; ++lineLimit) {
      for (int partitionCode = 0; partitionCode < CodecParams.MAX_PARTITION_CODE; ++partitionCode) {
        Encoder.Variant variant = new Encoder.Variant();
        variant.lineLimit = lineLimit;
        variant.partitionCode = partitionCode;
        variant.colorOptions = ~0L;
        params.variants.add(variant);
      }
    }
    params.targetSize = 287;
    params.numThreads = 1;
    for (String arg : args) {
      if (arg.startsWith("-t")) {
        params.targetSize = Integer.parseInt(arg.substring(2));
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
      } else if (arg.equals("-j")) {
        params.numThreads = Integer.parseInt(arg.substring(2));
        continue;
      }
      String path = arg;

      if (mode == Mode.ENCODE || mode == Mode.ENCODE_DECODE) {
        long t0 = System.nanoTime();
        BufferedImage image = ImageIO.read(new File(path));
        Encoder.Result result = Encoder.encode(image, params);
        long t1 = System.nanoTime();
        System.out.println("Time elapsed: " + (t1 - t0) / 1000000 + "ms");
        path = path + ".2im";
        Files.write(FileSystems.getDefault().getPath(path), result.data);
      }

      if (mode == Mode.DECODE || mode == Mode.ENCODE_DECODE) {
        byte[] data = Files.readAllBytes(FileSystems.getDefault().getPath(path));
        path = path + ".png";
        ImageIO.write(Decoder.decode(data), "png", new File(path));
      }
    }
  }
}
