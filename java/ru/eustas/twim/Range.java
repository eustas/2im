package ru.eustas.twim;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

final class Range {

  // Collection of classes. Should not be instantiated.
  Range() {}

  private static final int NUM_NIBBLES = 6;
  private static final int NIBBLE_BITS = 8;
  private static final int NIBBLE_MASK = (1 << NIBBLE_BITS) - 1;
  private static final int VALUE_BITS = NUM_NIBBLES * NIBBLE_BITS;
  private static final long VALUE_MASK = (1L << VALUE_BITS) - 1L;
  private static final int HEAD_NIBBLE_SHIFT = VALUE_BITS - NIBBLE_BITS;
  private static final long HEAD_START = 1L << HEAD_NIBBLE_SHIFT;
  private static final int RANGE_LIMIT_BITS = HEAD_NIBBLE_SHIFT - NIBBLE_BITS;
  private static final long RANGE_LIMIT_MASK = (1L << RANGE_LIMIT_BITS) - 1L;

  static final class Decoder {
    private long low;
    private long range = VALUE_MASK;
    private long code;
    private byte[] data;
    private int offset;

    Decoder(byte[] data) {
      this.data = data;
      long code = 0;
      for (int i = 0; i < NUM_NIBBLES; ++i) {
        code = (code << NIBBLE_BITS) | readNibble();
      }
      this.code = code;
    }

    private int readNibble() {
      return (offset < data.length) ? (data[offset++] & NIBBLE_MASK) : 0;
    }

    final void removeRange(int bottom, int top) {
      low += bottom * range;
      range *= top - bottom;
      while (true) {
        if ((low ^ (low + range - 1)) >= HEAD_START) {
          if (range > RANGE_LIMIT_MASK) {
            break;
          }
          range = -low & RANGE_LIMIT_MASK;
        }
        code = ((code << NIBBLE_BITS) & VALUE_MASK) | readNibble();
        range = (range << NIBBLE_BITS) & VALUE_MASK;
        low = (low << NIBBLE_BITS) & VALUE_MASK;
      }
    }

    final int currentCount(int totalRange) {
      range /= totalRange;
      return (int)((code - low) / range);
    }
  }

  static final class Encoder {
    private long low;
    private long range = VALUE_MASK;
    private ByteArrayOutputStream data = new ByteArrayOutputStream();

    final void encodeRange(int bottom, int top, int totalRange) {
      range /= totalRange;
      low += bottom * range;
      range *= top - bottom;
      while (true) {
        if ((low ^ (low + range - 1)) >= HEAD_START) {
          if (range > RANGE_LIMIT_MASK) {
            break;
          }
          range = -low & RANGE_LIMIT_MASK;
        }
        data.write((byte) (low >> HEAD_NIBBLE_SHIFT));
        range = (range << NIBBLE_BITS) & VALUE_MASK;
        low = (low << NIBBLE_BITS) & VALUE_MASK;
      }
    }

    final byte[] finish() {
      for (int i = 0; i < NUM_NIBBLES; ++i) {
        data.write((byte)(low >> HEAD_NIBBLE_SHIFT));
        low = (low << NIBBLE_BITS) & VALUE_MASK;
      }
      return data.toByteArray();
    }
  }

  static final class OptimalEncoder {
    Encoder encoder = new Encoder();
    List<Integer> raw = new ArrayList<Integer>();

    final void encodeRange(int bottom, int top, int totalRange) {
      raw.add(bottom);
      raw.add(top);
      raw.add(totalRange);
      encoder.encodeRange(bottom, top, totalRange);
    }

    private static int checkData(byte[] data, List<Integer> raw) {
      Decoder d = new Decoder(data);
      for (int i = 0; i < raw.size(); i += 3) {
        int bottom = raw.get(i);
        int top = raw.get(i + 1);
        int totalRange = raw.get(i + 2);
        int c = d.currentCount(totalRange);
        if (c < bottom) {
          return -1;
        } else if (c >= top) {
          return 1;
        }
        d.removeRange(bottom, top);
      }
      return 0;
    }

    final byte[] finish() {
      byte[] data = this.encoder.finish();
      int dataLength = data.length;
      while (dataLength > 0) {
        byte lastByte = data[--dataLength];
        data[dataLength] = 0;
        if (lastByte == 0 || checkData(data, raw) == 0) {
          continue;
        }
        int offset = dataLength - 1;
        while (offset >= 0 && data[offset] == -1) {
          offset--;
        }
        if (offset < 0) {
          data[dataLength++] = lastByte;
          break;
        }
        data[offset]++;
        for (int i = offset + 1; i < dataLength; ++i) {
          data[i] = 0;
        }
        if (checkData(data, this.raw) == 0) {
          continue;
        }
        data[offset]--;
        for (int i = offset + 1; i < dataLength; ++i) {
          data[i] = -1;
        }
        data[dataLength++] = lastByte;
        break;
      }
      byte[] result = new byte[dataLength];
      System.arraycopy(data, 0, result, 0, dataLength);
      return result;
    }
  }
}
