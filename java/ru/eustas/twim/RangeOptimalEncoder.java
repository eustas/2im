package ru.eustas.twim;

import java.util.ArrayList;
import java.util.List;

final class RangeOptimalEncoder {
  private final RangeEncoder encoder = new RangeEncoder();
  private final List<Integer> raw = new ArrayList<>();

  final void encodeRange(int bottom, int top, int totalRange) {
    raw.add(bottom);
    raw.add(top);
    raw.add(totalRange);
    encoder.encodeRange(bottom, top, totalRange);
  }

  private static int checkData(byte[] data, List<Integer> raw) {
    RangeDecoder d = new RangeDecoder(data);
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
