package ru.eustas.twim;

import static ru.eustas.twim.RangeCode.HEAD_NIBBLE_SHIFT;
import static ru.eustas.twim.RangeCode.HEAD_START;
import static ru.eustas.twim.RangeCode.NIBBLE_BITS;
import static ru.eustas.twim.RangeCode.NIBBLE_MASK;
import static ru.eustas.twim.RangeCode.NUM_NIBBLES;
import static ru.eustas.twim.RangeCode.RANGE_LIMIT_MASK;
import static ru.eustas.twim.RangeCode.VALUE_MASK;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

final class RangeEncoder {
  private final List<Integer> triplets = new ArrayList<>();

  static void writeNumber(RangeEncoder dst, int max, int value) {
    if (max == 1) return;
    dst.encodeRange(value, value + 1, max);
  }

  static void writeSize(RangeEncoder dst, int value) {
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

  final void encodeRange(int bottom, int top, int totalRange) {
    triplets.add(totalRange);
    triplets.add(bottom);
    triplets.add(top);
  }

  private byte[] encode() {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    long low = 0;
    long range = VALUE_MASK;
    int tripletsSize = triplets.size();
    for (int i = 0; i < tripletsSize;) {
      int totalRange = triplets.get(i++);
      int bottom = triplets.get(i++);
      int top = triplets.get(i++);
      range /= totalRange;
      low += bottom * range;
      range *= top - bottom;
      while (true) {
        if ((low ^ (low + range - 1)) >= HEAD_START) {
          if (range > RANGE_LIMIT_MASK) {
            break;
          }
          range = -low & VALUE_MASK;
        }
        out.write((int) (low >> HEAD_NIBBLE_SHIFT));
        range = ((range << NIBBLE_BITS) & VALUE_MASK) | NIBBLE_MASK;
        low = (low << NIBBLE_BITS) & VALUE_MASK;
      }
    }
    for (int i = 0; i < NUM_NIBBLES; ++i) {
      out.write((int) (low >> HEAD_NIBBLE_SHIFT));
      low = (low << NIBBLE_BITS) & VALUE_MASK;
    }
    return out.toByteArray();
  }

  private static class Decoder {
    private byte[] data;
    private int dataLength;
    private int offset;
    private long code;
    private long low;
    private long range;

    private void init(byte[] data, int dataLength) {
      this.data = data;
      this.dataLength = dataLength;
    }

    private void copy(Decoder other) {
      this.offset = other.offset;
      this.code = other.code;
      this.low = other.low;
      this.range = other.range;
    }

    boolean decodeRange(int totalRange, int bottom, int top) {
      range /= totalRange;
      int count = (int) ((code - low) / range);
      if ((count < bottom) || (count >= top)) {
        return false;
      }
      low += bottom * range;
      range *= top - bottom;
      while (true) {
        if ((low ^ (low + range - 1)) >= HEAD_START) {
          if (range > RANGE_LIMIT_MASK) {
            break;
          }
          range = -low & VALUE_MASK;
        }
        long nibble = (offset < dataLength) ? (data[offset++] & 0xFF) : 0;
        code = ((code << NIBBLE_BITS) & VALUE_MASK) | nibble;
        range = ((range << NIBBLE_BITS) & VALUE_MASK) | NIBBLE_MASK;
        low = (low << NIBBLE_BITS) & VALUE_MASK;
      }
      return true;
    }
  }

  private byte[] optimize(byte[] data) {
    // KISS
    if (data.length <= 2 * NUM_NIBBLES) {
      return data;
    }

    Decoder current = new Decoder();
    current.init(data, data.length);
    for (int i = 0; i < NUM_NIBBLES; ++i) {
      current.code = (current.code << NIBBLE_BITS) | (current.data[current.offset++] & 0xFF);
    }
    current.range = VALUE_MASK;

    Decoder good = new Decoder();
    good.copy(current);
    int tripletsSize = triplets.size();
    int i = 0;
    while (i < tripletsSize) {
      current.decodeRange(triplets.get(i), triplets.get(i + 1), triplets.get(i + 2));
      if (current.offset + 2 * NUM_NIBBLES > data.length) {
        break;
      }
      good.copy(current);
      i += 3;
    }

    int bestCut = 0;
    int bestCutDelta = 0;
    for (int cut = 1; cut <= NUM_NIBBLES; ++cut) {
      current.dataLength = data.length - cut;
      byte originalTail = data[current.dataLength - 1];
      for (int delta = -1; delta <= 1; ++delta) {
        current.copy(good);
        data[current.dataLength - 1] = (byte) (originalTail + delta);
        int j = i;
        boolean ok = true;
        while (ok && (j < tripletsSize)) {
          ok = current.decodeRange(triplets.get(j), triplets.get(j + 1), triplets.get(j + 2));
          j += 3;
        }
        if (ok) {
          bestCut = cut;
          bestCutDelta = delta;
        }
      }
      data[current.dataLength - 1] = originalTail;
    }
    data = Arrays.copyOf(data, data.length - bestCut);
    data[data.length - 1] = (byte) (data[data.length - 1] + bestCutDelta);
    return data;
  }

  final byte[] finish() {
    return optimize(encode());
  }
}
