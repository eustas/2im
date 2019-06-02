package ru.eustas.twim;

class CodecParams {
  private static final int MAX_LEVEL = 7;

  private static final int MAX_F1 = 4;
  private static final int MAX_F2 = 5;
  private static final int MAX_F3 = 5;
  private static final int MAX_F4 = 5;

  static final int MAX_PARTITION_CODE = MAX_F1 * MAX_F2 * MAX_F3 * MAX_F4;
  static final int MAX_COLOR_CODE = 17;

  static final int MAX_CODE = MAX_PARTITION_CODE * MAX_COLOR_CODE;

  static int makeColorQuant(int code) {
    return 1 + ((4 + (code & 3)) << (code >> 2));
  }

  static int dequantizeColor(int v, int q) {
    return (255 * v + q - 2) / (q - 1);
  }

  private static final int[] SCALE_STEP = {1000, 631, 399, 252, 159, 100};
  private static final int SCALE_STEP_FACTOR = 40;
  private static final int BASE_SCALE_FACTOR = 36;

  // Not the best place, but OK for prototype.
  final static int NODE_FILL = 0;
  final static int NODE_HALF_PLANE = 1;
  final static int NODE_TYPE_COUNT = 2;

  private final int[] levelScale = new int[MAX_LEVEL];
  final int[] angleBits = new int[MAX_LEVEL];
  int colorQuant;

  // Actually, that is image params...
  final int width;
  final int height;
  final int lineLimit = 25;

  private int partitionCode;
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
    colorQuant = makeColorQuant(code);
  }

  void setPartitionCode(int code) {
    partitionCode = code;
    int f1 = code % MAX_F1;
    code = code / MAX_F1;
    int f2 = 2 + code % MAX_F2;
    code = code / MAX_F2;
    int f3 = code % MAX_F3;
    code = code / MAX_F3;
    int f4 = code % MAX_F4;

    int scale = (width * width + height * height) * f2 * f2;
    for (int i = 0; i < MAX_LEVEL; ++i) {
      levelScale[i] = scale / BASE_SCALE_FACTOR;
      scale = (scale * SCALE_STEP_FACTOR) / SCALE_STEP[f3];
    }

    int bits = 9 - f1;
    for (int i = 0; i < MAX_LEVEL; ++i) {
      angleBits[i] = Math.max(bits - i - (i * f4) / 2, 0);
    }
  }

  public String toString() {
    int code = partitionCode;
    int f1 = code % MAX_F1;
    code = code / MAX_F1;
    int f2 = code % MAX_F2;
    code = code / MAX_F2;
    int f3 = code % MAX_F3;
    code = code / MAX_F3;
    int f4 = code % MAX_F4;
    return "p: " + f1 + "" + f2 + "" + f3 + "" + f4 + ", clr: " + colorCode;
  }

  int getCode() {
    return partitionCode + colorCode * MAX_PARTITION_CODE;
  }

  void setCode(int code) {
    setPartitionCode(code % MAX_PARTITION_CODE);
    code /= MAX_PARTITION_CODE;
    setColorCode(code % MAX_COLOR_CODE);
  }

  int getLevel(int[] region) {
    final int count3 = region[region.length - 1] * 3;
    if (count3 == 0) {
      return -1;
    }

    // TODO(eustas): perhaps box area is not the best value for level calculation.
    int min_y = height + 1;
    int max_y = -1;
    int min_x = width + 1;
    int max_x = -1;
    for (int i = 0; i < count3; i += 3) {
      int y = region[i];
      int x0 = region[i + 1];
      int x1 = region[i + 2];
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
