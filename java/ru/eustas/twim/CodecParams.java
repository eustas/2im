package ru.eustas.twim;

import java.util.Arrays;

import static ru.eustas.twim.XRangeDecoder.readNumber;
import static ru.eustas.twim.XRangeEncoder.writeNumber;

class CodecParams {
  private static final int MAX_LEVEL = 7;

  private static final int MAX_F1 = 4;
  private static final int MAX_F2 = 5;
  private static final int MAX_F3 = 5;
  private static final int MAX_F4 = 5;
  static final int MAX_LINE_LIMIT = 63;

  static final int MAX_PARTITION_CODE = MAX_F1 * MAX_F2 * MAX_F3 * MAX_F4;
  static final int MAX_COLOR_QUANT_OPTIONS = 17;
  static final int MAX_COLOR_PALETTE_SIZE = 32;
  static final int MAX_COLOR_CODE = MAX_COLOR_QUANT_OPTIONS + MAX_COLOR_PALETTE_SIZE;
  static final int TAX = MAX_PARTITION_CODE * MAX_LINE_LIMIT * MAX_COLOR_CODE;

  static int makeColorQuant(int code) {
    return 1 + ((4 + (code & 3)) << (code >> 2));
  }

  static int dequantizeColor(int v, int q) {
    int vMax = q - 1;
    return 255 * v / vMax;
  }

  static int readSize(XRangeDecoder src) {
    int plus = -1;
    int bits = 0;
    while ((plus <= 0) || (readNumber(src, 2) == 1)) {
      plus = (plus + 1) << 3;
      int extra = readNumber(src, 8);
      bits = (bits << 3) + extra;
    }
    return bits + plus;
  }

  static void writeSize(XRangeEncoder dst, int value) {
    value -= 8;
    int chunks = 2;
    while (value > (1 << (chunks * 3))) {
      value -= (1 << (chunks * 3));
      chunks++;
    }
    for (int i = 0; i < chunks; ++i) {
      if (i > 1) {
        writeNumber(dst, 2, 1);
      }
      writeNumber(dst, 8, (value >> (3 * (chunks - i - 1))) & 7);
    }
    writeNumber(dst, 2, 0);
  }

  private static final int SCALE_STEP_FACTOR = 40;
  private static final int BASE_SCALE_FACTOR = 36;

  // Not the best place, but OK for prototype.
  final static int NODE_FILL = 0;
  final static int NODE_HALF_PLANE = 1;
  final static int NODE_TYPE_COUNT = 2;

  private final int[] levelScale = new int[MAX_LEVEL];
  final int[] angleBits = new int[MAX_LEVEL];
  int colorQuant;
  int paletteSize;

  // Actually, that is image params...
  final int width;
  final int height;
  int lineLimit = 63;

  private int[] params;
  private int colorCode;

  CodecParams(int width, int height) {
    this.width = width;
    this.height = height;
  }

  int getLineQuant() {
    return SinCos.SCALE;
  }

  void setColorCode(int code) {
    colorCode = code;
    if (colorCode < MAX_COLOR_QUANT_OPTIONS) {
      colorQuant = makeColorQuant(code);
      paletteSize = 0;
    } else {
      colorQuant = 0;
      paletteSize = colorCode - MAX_COLOR_QUANT_OPTIONS + 1;
    }
  }

  private int[] splitCode(int code) {
    int[] result = new int[4];
    result[0] = code % MAX_F1;
    code = code / MAX_F1;
    result[1] = code % MAX_F2;
    code = code / MAX_F2;
    result[2] = code % MAX_F3;
    code = code / MAX_F3;
    result[3] = code % MAX_F4;
    return result;
  }

  void setPartitionCode(int code) {
    setPartitionParams(splitCode(code));
  }

  private void setPartitionParams(int[] params) {
    this.params = Arrays.copyOf(params, 4);
    int f1 = params[0];
    int f2 = params[1] + 2;
    int f3 = (int) Math.pow(10, 3 - params[2] / 5.0);
    int f4 = params[3];
    int scale = (width * width + height * height) * f2 * f2;
    for (int i = 0; i < MAX_LEVEL; ++i) {
      levelScale[i] = scale / BASE_SCALE_FACTOR;
      scale = (scale * SCALE_STEP_FACTOR) / f3;
    }

    int bits = SinCos.MAX_ANGLE_BITS - f1;
    for (int i = 0; i < MAX_LEVEL; ++i) {
      angleBits[i] = Math.max(bits - i - (i * f4) / 2, 0);
    }
  }

  public String toString() {
    return "p: " + params[0] + "" + params[1] + "" + params[2] + "" + params[3]
        + ", l: " + lineLimit + ", c: " + colorCode;
  }

  static CodecParams read(XRangeDecoder src) {
    int width = readSize(src);
    int height = readSize(src);
    CodecParams cp = new CodecParams(width, height);
    int[] params = {readNumber(src, MAX_F1), readNumber(src, MAX_F2), readNumber(src, MAX_F3), readNumber(src, MAX_F4)};
    cp.setPartitionParams(params);
    cp.lineLimit = readNumber(src, MAX_LINE_LIMIT) + 1;
    cp.setColorCode(readNumber(src, MAX_COLOR_CODE));
    return cp;
  }

  void write(XRangeEncoder dst) {
    writeSize(dst, width);
    writeSize(dst, height);
    writeNumber(dst, MAX_F1, params[0]);
    writeNumber(dst, MAX_F2, params[1]);
    writeNumber(dst, MAX_F3, params[2]);
    writeNumber(dst, MAX_F4, params[3]);
    writeNumber(dst, MAX_LINE_LIMIT, lineLimit - 1);
    writeNumber(dst, MAX_COLOR_CODE, colorCode);
  }

  int getLevel(int[] region) {
    final int step = region.length / 3;
    final int count = region[region.length - 1];
    if (count == 0) {
      return -1;
    }

    // TODO(eustas): perhaps box area is not the best value for level calculation.
    int min_y = height + 1;
    int max_y = -1;
    int min_x = width + 1;
    int max_x = -1;
    for (int i = 0; i < count; i++) {
      int y = region[i];
      int x0 = region[step + i];
      int x1 = region[2 * step + i];
      min_y = Math.min(min_y, y);
      max_y = Math.max(max_y, y);
      min_x = Math.min(min_x, x0);
      max_x = Math.max(max_x, x1);
    }

    int dx = max_x - min_x;
    int dy = max_y + 1 - min_y;
    int d = dx * dx + dy * dy;
    for (int i = 0; i < MAX_LEVEL; ++i) {
      if (d >= levelScale[i]) {
        return i;
      }
    }
    return MAX_LEVEL - 1;
  }
}
