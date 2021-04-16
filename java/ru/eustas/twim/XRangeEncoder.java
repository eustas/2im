package ru.eustas.twim;

import static ru.eustas.twim.XRangeCode.BITS;
import static ru.eustas.twim.XRangeCode.MAX;
import static ru.eustas.twim.XRangeCode.MIN;
import static ru.eustas.twim.XRangeCode.SPACE;

import java.util.ArrayList;
import java.util.Collections;

final class XRangeEncoder {
  private static int encodeNumber(int state, int value, int max, ArrayList<Byte> bits) {
    int low = value * SPACE;
    int base = low / max;
    int freq = (low + SPACE) / max - base;
    while (state >= MAX * freq / SPACE) {
      bits.add((byte)(state & 1));
      state >>= 1;
    }
    return ((state / freq) * SPACE) + (state % freq) + base;
  }

  private static class Entry {
    final int value;
    final int max;
    Entry(int value, int max) {
      this.value = value;
      this.max = max;
    }
  }

  private ArrayList<Entry> entries = new ArrayList<Entry>();

  static void writeNumber(XRangeEncoder dst, int max, int value) {
    // TODO(eustas): assert(max > 0)
    if (max > 1) dst.entries.add(new Entry(value, max));
  }

  byte[] finish() {
    // First of all, reverse the input.

    Collections.reverse(entries);
    ArrayList<Byte> bits = new ArrayList<Byte>(1024);

    // Calculate the "head" length.
    int limit = entries.size();
    double cost = 4.3e9; // ~2^32
    for (int i = 0; i < entries.size(); ++i) {
      cost /= entries.get(i).max;
      if (cost < 1.0) {
        limit = i + 1;
        break;
      }
    }

    int maxLeadingZeros = 0;
    int bestInitialState = MIN;
    for (int initialState = MIN; initialState < MAX + 0x1C; initialState += 32) {
      bits.clear();
      int state = initialState;
      for (int i = 0; i < limit; ++i) {
        Entry entry = entries.get(i);
        state = encodeNumber(state, entry.value, entry.max, bits);
      }
      int numLeadingZeros = bits.size();
      for (int i = 0; i < bits.size(); ++i) {
        if (bits.get(i) != 0) {
          numLeadingZeros = i;
          break;
        }
      }
      if (numLeadingZeros > maxLeadingZeros) {
        maxLeadingZeros = numLeadingZeros;
        bestInitialState = initialState;
      }
    }

   bits.clear();
   int state = bestInitialState;
    for (int i = 0; i < entries.size(); ++i) {
      Entry entry = entries.get(i);
      state = encodeNumber(state, entry.value, entry.max, bits);
    }
    for (int i = 0; i < BITS; ++i) {
      bits.add((byte)((state >> (BITS - 1 - i)) & 1));
    }

    // Now reverse the output. Double reverse -> first bits define first values.
    Collections.reverse(bits);
    // Remove tail of zeroes.
    while (!bits.isEmpty() && (bits.get(bits.size() - 1) == 0)) {
      bits.remove(bits.size() - 1);
    }

    // But add some padding zeroes back for byte-alignment.
    while ((bits.size() & 7) != 0) {
      bits.add((byte)0);
    }

    int numBytes = bits.size() >> 3;
    byte[] out = new byte[numBytes];
    for (int i = 0; i < numBytes; ++i) {
      byte b = 0;
      for (int j = 0; j < 8; ++j) b |= bits.get(8 * i + j) << j;
      out[i] = b;
    }
    return out;
  }
}
