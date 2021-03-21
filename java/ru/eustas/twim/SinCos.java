package ru.eustas.twim;

class SinCos {
  static final int SCALE = 1 << 18;

  static final int MAX_ANGLE_BITS = 9;
  static final int MAX_ANGLE = 1 << MAX_ANGLE_BITS;

  static final int[] SIN = new int[MAX_ANGLE];
  static final int[] COS = new int[MAX_ANGLE];
  static final double[] INV_SIN = new double[MAX_ANGLE];
  static final float[] MINUS_COT = new float[MAX_ANGLE];

  static {
    for (int i = 0; i < MAX_ANGLE; ++i) {
      SIN[i] = (int) Math.round(SCALE * Math.sin((Math.PI * i) / MAX_ANGLE));
      COS[i] = (int) Math.round(SCALE * Math.cos((Math.PI * i) / MAX_ANGLE));
      INV_SIN[i] = (i != 0) ? (1.0 / SIN[i]) : 0.0;
      MINUS_COT[i] = (float) (-COS[i] * INV_SIN[i]);
    }
  }
}
