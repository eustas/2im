package ru.eustas.twim;

import jdk.incubator.vector.FloatVector;
import jdk.incubator.vector.IntVector;
import jdk.incubator.vector.VectorSpecies;

public class EncoderSimdCore implements EncoderCore {
  private static final VectorSpecies<Float> VFP = FloatVector.SPECIES_256;
  private static final VectorSpecies<Integer> VIP = IntVector.SPECIES_256;
  private static final VectorSpecies<Integer> VI4 = IntVector.SPECIES_256;

  EncoderSimdCore() {
    if (!Boolean.parseBoolean(System.getProperty("twim.encoder.use_simd"))) {
      throw new Error("SIMD core is not enabled");
    }
  }

  static final int STEP = VFP.length();
  public int padding() {
    return STEP;
  }

  public void sumAbs(int[] sum, int count, int[] regionX, int[] dst) {
    if (count > regionX.length) return;
    IntVector acc1 = IntVector.fromArray(VI4, sum, regionX[0] * 4);
    for (int i = 1; i < count; i += 1) {
      acc1 = acc1.add(IntVector.fromArray(VI4, sum, regionX[i] * 4));
    }
    acc1.intoArray(dst, 0);
  }

  private static final int MAX_INT = (1 << 23) - 1;
  private static final IntVector INTEGER_MASK = IntVector.broadcast(VIP, MAX_INT);
  private static final FloatVector IMPLICIT_ONE = FloatVector.broadcast(VFP, MAX_INT + 1);

  // x >= (d - y * ny) / nx
  public void updateGeGeneric(int angle, int d, float[] regionY, int[] regionX0, float[] regionX0f,
      int[] regionX1, float[] regionX1f, int[] rowOffset, int[] regionX, int count, int kappa) {
    FloatVector mNyNx = FloatVector.broadcast(VFP, SinCos.MINUS_COT[angle]);
    FloatVector dNx = FloatVector.broadcast(VFP, (float)(d * SinCos.INV_SIN[angle]));
    for (int i = 0; i < count; i += STEP) {
      FloatVector y = FloatVector.fromArray(VFP, regionY, i);
      FloatVector x0 = FloatVector.fromArray(VFP, regionX0f, i);
      FloatVector x1 = FloatVector.fromArray(VFP, regionX1f, i);
      IntVector off = IntVector.fromArray(VIP, rowOffset, i);
      FloatVector x = y.fma(mNyNx, dNx).max(x0).min(x1);
      IntVector xi = x.add(IMPLICIT_ONE).viewAsIntegralLanes().and(INTEGER_MASK);
      IntVector xOff = xi.add(off);
      xOff.intoArray(regionX, i);
    }
  }

  public void score(int[] whole, int n, float[] r, float[] g, float[] b, float[] c, float[] s) {
    FloatVector k1 = FloatVector.broadcast(VFP, 1);
    FloatVector wholeC = FloatVector.broadcast(VFP, whole[3]);
    FloatVector wholeInvC = k1.div(wholeC);
    FloatVector wholeR = FloatVector.broadcast(VFP, whole[0]);
    FloatVector wholeG = FloatVector.broadcast(VFP, whole[1]);
    FloatVector wholeB = FloatVector.broadcast(VFP, whole[2]);
    FloatVector wholeAvgR = wholeR.mul(wholeInvC);
    FloatVector wholeAvgG = wholeG.mul(wholeInvC);
    FloatVector wholeAvgB = wholeB.mul(wholeInvC);

    for (int i = 0; i < n; i += STEP) {
      FloatVector leftC = FloatVector.fromArray(VFP, c, i);
      FloatVector leftInvC = k1.div(leftC);
      FloatVector rightC = wholeC.sub(leftC);
      FloatVector rightInvC = k1.div(rightC);

      FloatVector leftR = FloatVector.fromArray(VFP, r, i);
      FloatVector leftAvgR = leftR.mul(leftInvC);
      FloatVector leftDiffR = leftAvgR.sub(wholeAvgR);
      FloatVector sumLeft = leftDiffR.mul(leftDiffR);
      FloatVector rightR = wholeR.sub(leftR);
      FloatVector rightAvgR = rightR.mul(rightInvC);
      FloatVector rightDiffR = rightAvgR.sub(wholeAvgR);
      FloatVector sumRight = rightDiffR.mul(rightDiffR);

      FloatVector leftG = FloatVector.fromArray(VFP, g, i);
      FloatVector leftAvgG = leftG.mul(leftInvC);
      FloatVector leftDiffG = leftAvgG.sub(wholeAvgG);
      sumLeft = leftDiffG.fma(leftDiffG, sumLeft);
      FloatVector rightG = wholeG.sub(leftG);
      FloatVector rightAvgG = rightG.mul(rightInvC);
      FloatVector rightDiffG = rightAvgG.sub(wholeAvgG);
      sumRight = rightDiffG.fma(rightDiffG, sumRight);

      FloatVector leftB = FloatVector.fromArray(VFP, b, i);
      FloatVector leftAvgB = leftB.mul(leftInvC);
      FloatVector leftDiffB = leftAvgB.sub(wholeAvgB);
      sumLeft = leftDiffB.fma(leftDiffB, sumLeft);
      FloatVector rightB = wholeB.sub(leftB);
      FloatVector rightAvgB = rightB.mul(rightInvC);
      FloatVector rightDiffB = rightAvgB.sub(wholeAvgB);
      sumRight = rightDiffB.fma(rightDiffB, sumRight);

      sumLeft.mul(leftC).add(sumRight.mul(rightC)).intoArray(s, i);
    }
  }
}
