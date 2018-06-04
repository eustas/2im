package ru.eustas.twim;

/**
 * CJK unicode to ordinal and ordinal to CJK unicode transformation.
 *
 * NB: CJK does not fit "Basic Multilingual Plane", i.e. can not be represented with "char".
 * To convert CJK to UTF-8 text and back use {@link UtfDecoder} and {@link UtfEncoder}.
 */
final class CjkTransform {

  /* Pairs of values representing first and last scalars in CJK ranges. */
  private static final int[] CJK = new int[] {
    0x04E00, 0x09FFF, 0x03400, 0x04DBF, 0x20000, 0x2A6DF, 0x2A700,
    0x2B73F, 0x2B740, 0x2B81F, 0x2B820, 0x2CEAF, 0x2CEB0, 0x2EBEF
  };

  // Collection of utilities. Should not be instantiated.
  CjkTransform() {}

  static int unicodeToOrdinal(int chr) {
    int sum = 0;
    for (int i = 0; i < CJK.length; i += 2) {
      int size = CJK[i + 1] - CJK[i] + 1;
      int delta = chr - CJK[i];
      if (delta >= 0 && delta < size) {
        return sum + delta;
      }
      sum += size;
    }
    return -1;
  }

  static int ordinalToUnicode(int ord) {
    int sum = 0;
    for (int i = 0; i < CJK.length; i += 2) {
      int size = CJK[i + 1] - CJK[i] + 1;
      int delta = ord - sum;
      if (delta < size) {
        return CJK[i] + delta;
      }
      sum += size;
    }
    return -1;
  }
}