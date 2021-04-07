package ru.eustas.twim;

public class EncoderVanillaCore implements EncoderCore {
  public int padding() {
    return 1;
  }

  public void sumAbs(int[] sum, int count, int[] regionX, int[] dst) {
    //for (int j = 0; j < 4; ++j) dst[j] = 0;
    int a = 0, b = 0, c = 0, d = 0;
    for (int i = 0; i < count; i++) {
      int offset = regionX[i];
      //for (int j = 0; j < 4; ++j) dst[j] += sum[offset * 4 + j];
      a += sum[offset * 4 + 0];
      b += sum[offset * 4 + 1];
      c += sum[offset * 4 + 2];
      d += sum[offset * 4 + 3];
    }
    dst[0] = a;
    dst[1] = b;
    dst[2] = c;
    dst[3] = d;
  }

   // x >= (d - y * ny) / nx
   public void updateGeGeneric(int angle, int d, float[] regionY, int[] regionX0, float[] regionX0f,
   int[] regionX1, float[] regionX1f, int[] rowOffset, int[] regionX, int count, int kappa) {
      float mNyNx = SinCos.MINUS_COT[angle];
      float dNx = (float)(d * SinCos.INV_SIN[angle] + 0.5f);
      for (int i = 0; i < count; i++) {
        float y = regionY[i];
        int offset = rowOffset[i];
        float xf = y * mNyNx + dNx;
        int xi = (int)xf;
        int x0 = regionX0[i];
        int x1 = regionX1[i];
        int x = Math.min(x1, Math.max(xi, x0));
        int xOff = x + offset;
        regionX[i] = xOff;
      }
  }

  public void score(int[] whole, int n, float[] r, float[] g, float[] b, float[] c, float[] s) {
    float wholeC = whole[3];
    float wholeInvC = 1.0f / wholeC;
    float wholeR = whole[0];
    float wholeG = whole[1];
    float wholeB = whole[2];
    float wholeAvgR = wholeR * wholeInvC;
    float wholeAvgG = wholeG * wholeInvC;
    float wholeAvgB = wholeB * wholeInvC;

    for (int i = 0; i < n; i++) {
      float leftC = c[i];
      float leftInvC = 1.0f / leftC;
      float rightC = wholeC - leftC;
      float rightInvC = 1.0f / rightC;

      float leftR = r[i];
      float leftAvgR = leftR * leftInvC;
      float leftDiffR = leftAvgR - wholeAvgR;
      float sumLeft = leftDiffR * leftDiffR;
      float rightR = wholeR - leftR;
      float rightAvgR = rightR * rightInvC;
      float rightDiffR = rightAvgR - wholeAvgR;
      float sumRight = rightDiffR * rightDiffR;

      float leftG = g[i];
      float leftAvgG = leftG * leftInvC;
      float leftDiffG = leftAvgG - wholeAvgG;
      sumLeft += leftDiffG * leftDiffG;
      float rightG = wholeG - leftG;
      float rightAvgG = rightG * rightInvC;
      float rightDiffG = rightAvgG - wholeAvgG;
      sumRight += rightDiffG * rightDiffG;

      float leftB = b[i];
      float leftAvgB = leftB * leftInvC;
      float leftDiffB = leftAvgB - wholeAvgB;
      sumLeft += leftDiffB * leftDiffB;
      float rightB = wholeB - leftB;
      float rightAvgB = rightB * rightInvC;
      float rightDiffB = rightAvgB - wholeAvgB;
      sumRight += rightDiffB * rightDiffB;

      s[i] = sumLeft * leftC + sumRight * rightC;
    }
  }
}
