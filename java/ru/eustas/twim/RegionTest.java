package ru.eustas.twim;

import org.junit.Test;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class RegionTest {
  @Test
  public void testHorizontalSplit() {
    int[] region = {0, 0, 4, 1};
    int angle = SinCos.MAX_ANGLE / 2;
    Region.DistanceRange distanceRange = new Region.DistanceRange();
    distanceRange.update(region, angle, SinCos.SCALE);
    assertEquals(3, distanceRange.numLines);
    int[] left = new int[4];
    int[] right = new int[4];
    // 1/3
    Region.splitLine(region, angle, distanceRange.distance(0), left, right);
    assertArrayEquals(new int[]{0, 1, 4, 1}, left);
    assertArrayEquals(new int[]{0, 0, 1, 1}, right);
    // 2/3
    Region.splitLine(region, angle, distanceRange.distance(1), left, right);
    assertArrayEquals(new int[]{0, 2, 4, 1}, left);
    assertArrayEquals(new int[]{0, 0, 2, 1}, right);
    // 3/3
    Region.splitLine(region, angle, distanceRange.distance(2), left, right);
    assertArrayEquals(new int[]{0, 3, 4, 1}, left);
    assertArrayEquals(new int[]{0, 0, 3, 1}, right);
  }

  @Test
  public void testVerticalSplit() {
    int[] region = {0, 0, 1, 1, 0, 1, 2, 0, 1, 3, 0, 1, 4};
    int angle = 0;
    Region.DistanceRange distanceRange = new Region.DistanceRange();
    distanceRange.update(region, angle, SinCos.SCALE);
    assertEquals(3, distanceRange.numLines);
    {
      // 1/3
      int[] left = new int[10];
      int[] right = new int[4];
      Region.splitLine(region, angle, distanceRange.distance(0), left, right);
      assertArrayEquals(new int[]{1, 0, 1, 2, 0, 1, 3, 0, 1, 3}, left);
      assertArrayEquals(new int[]{0, 0, 1, 1}, right);
    }
    {
      // 2/3
      int[] left = new int[7];
      int[] right = new int[7];
      Region.splitLine(region, angle, distanceRange.distance(1), left, right);
      assertArrayEquals(new int[]{2, 0, 1, 3, 0, 1, 2}, left);
      assertArrayEquals(new int[]{0, 0, 1, 1, 0, 1, 2}, right);
    }
    {
      int[] left = new int[4];
      int[] right = new int[10];
      Region.splitLine(region, angle, distanceRange.distance(2), left, right);
      assertArrayEquals(new int[]{3, 0, 1, 1}, left);
      assertArrayEquals(new int[]{0, 0, 1, 1, 0, 1, 2, 0, 1, 3}, right);
      // 3/3
    }
  }
}
