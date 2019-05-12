package ru.eustas.twim;

public class SinCos {
  public static final int SCALE_BITS = 20;
  public static final int SCALE = 1 << SCALE_BITS;

  public static final int MAX_ANGLE_BITS = 9;
  public static final int MAX_ANGLE = 1 << MAX_ANGLE_BITS;

  public static final int[] SIN = new int[MAX_ANGLE];
  public static final int[] COS = new int[MAX_ANGLE];

  static {
    for (int i = 0; i < MAX_ANGLE; ++i) {
      SIN[i] = (int) Math.round(SCALE * Math.sin((Math.PI * i) / MAX_ANGLE));
      COS[i] = (int) Math.round(SCALE * Math.cos((Math.PI * i) / MAX_ANGLE));
    }
  }
}
