package ru.eustas.twim;

class SinCos {
  static final int SCALE = 1 << 18;

  static final int MAX_ANGLE_BITS = 9;
  static final int MAX_ANGLE = 1 << MAX_ANGLE_BITS;

  static final int[] SIN = new int[MAX_ANGLE];
  static final int[] COS = new int[MAX_ANGLE];

  static {
    for (int i = 0; i < MAX_ANGLE; ++i) {
      SIN[i] = (int) Math.round(SCALE * Math.sin((Math.PI * i) / MAX_ANGLE));
      COS[i] = (int) Math.round(SCALE * Math.cos((Math.PI * i) / MAX_ANGLE));
    }
  }
}
