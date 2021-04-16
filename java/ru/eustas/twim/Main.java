package ru.eustas.twim;

import javax.imageio.ImageIO;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.util.List;

public class Main {
  enum Mode {
    DECODE,
    ENCODE,
    ENCODE_DECODE,
  }

  private static void fillAllVariants(List<Encoder.Variant> variants) {
    variants.clear();
    for (int lineLimit = 0; lineLimit < CodecParams.MAX_LINE_LIMIT; ++lineLimit) {
      for (int partitionCode = 0; partitionCode < CodecParams.MAX_PARTITION_CODE; ++partitionCode) {
        Encoder.Variant variant = new Encoder.Variant();
        variant.lineLimit = lineLimit;
        variant.partitionCode = partitionCode;
        variant.colorOptions = ~0L;
        variants.add(variant);
      }
    }
  }

  // See https://twitter.com/eustasru/status/1279418273612865536
  // N = 19; tuples = [((1 << i) % N, i) for i in range(N - 1)]
  // print([N] + [t[1] for t in sorted(tuples, key=lambda x: x[0])])
  private static final int[] POW_2_REVERSE =
      new int[] {99, 0, 1, 13, 2, 99, 14, 6, 3, 8, 99, 12, 15, 5, 7, 11, 4, 10, 9};

  private static void parseVariants(String arg, List<Encoder.Variant> variants) {
    variants.clear();
    String[] chunks = arg.split(",", -1);
    for (String chunk : chunks) {
      if (chunk.length() < 5) throw new IllegalArgumentException();
      String[] parts = chunk.split(":", -1);
      if (parts.length != 3) throw new IllegalArgumentException();
      int partitionCode = Integer.parseInt(parts[0], 16);
      if (partitionCode < 0 || partitionCode >= CodecParams.MAX_PARTITION_CODE) throw new IllegalArgumentException();
      long lineLimit = "*".equals(parts[1]) ? ((1L << CodecParams.MAX_LINE_LIMIT) - 1) : Long.parseLong(parts[1], 16);
      if (lineLimit < 0 || (lineLimit >>> CodecParams.MAX_LINE_LIMIT) != 0) throw new IllegalArgumentException();
      long colorOptions = "*".equals(parts[2]) ? ((1L << CodecParams.MAX_COLOR_CODE) - 1) : Long.parseLong(parts[2], 16);
      for (int i = 0; i < 4; ++i) {
        int word = (int)(lineLimit >>> (i * 16)) & 0xFFFF;
        while (word != 0) {
          int newWord = word & (word - 1);
          int bit = word ^ newWord;
          word = newWord;
          int bitPos = POW_2_REVERSE[bit % 19] + 16 * i;
          Encoder.Variant v = new Encoder.Variant();
          v.partitionCode = partitionCode;
          v.lineLimit = bitPos;
          v.colorOptions = colorOptions;
          variants.add(v);
        }
      }
    }
  }

  public static void main(String... args) throws IOException {
    Mode mode = Mode.ENCODE;
    Encoder.Params params = new Encoder.Params();
    fillAllVariants(params.variants);
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
      } else if (arg.startsWith("-j")) {
        params.numThreads = Integer.parseInt(arg.substring(2));
        continue;
      } else if (arg.startsWith("-p")) {
        parseVariants(arg.substring(2), params.variants);
        continue;
      }
      String path = arg;

      if (mode == Mode.ENCODE || mode == Mode.ENCODE_DECODE) {
        System.out.println("Encdoing " + path);
        long t0 = System.nanoTime();
        BufferedImage image = ImageIO.read(new File(path));
        Encoder.Result result = Encoder.encode(image, params);
        long t1 = System.nanoTime();
        System.out.println("Time elapsed: " + (t1 - t0) / 1000000 + "ms");
        path = path + ".2im";
        Files.write(FileSystems.getDefault().getPath(path), result.data);
      }

      if (mode == Mode.DECODE || mode == Mode.ENCODE_DECODE) {
        System.out.println("Decoding " + path);
        byte[] data = Files.readAllBytes(FileSystems.getDefault().getPath(path));
        path = path + ".png";
        BufferedImage image = Decoder.decode(data);
        if (image == null) {
          System.out.println("Corrupted stream");
        } else {
          ImageIO.write(image, "png", new File(path));
        }
      }
    }
  }
}
