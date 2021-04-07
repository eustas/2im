package ru.eustas.twim;

public interface EncoderCore {
  int padding();
  void sumAbs(int[] sum, int count, int[] regionX, int[] dst);
  void updateGeGeneric(int angle, int d, float[] regionY, int[] regionX0, float[] regionX0f,
      int[] regionX1, float[] regionX1f, int[] rowOffset, int[] regionX, int count, int kappa);
  void score(int[] whole, int n, float[] r, float[] g, float[] b, float[] c, float[] s);
}
