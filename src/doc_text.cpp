#include "doc_text.hpp"

static const uchar *get_piece_table(const Storage &storage) {
  const uchar *wds, *ts;
  uint wds_size, ts_size;
  if (!storage.stream("WordDocument", &wds, &wds_size)) {
    return NULL;
  }
  ushort flag1 = *(const ushort *)(wds + 0x0A);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  if (!storage.stream(is1Table ? "1Table" : "0Table", &ts, &ts_size)) {
    return NULL;
  }

  uint fcClx = *(const uint *)(wds + 0x01A2);
  const uchar *clx = ts + fcClx;
  for (uint pos = 0; clx + pos < wds + wds_size;) {
    uchar type = clx[pos];
    if (type == 2) {
      return clx + pos;
    } else if (type == 1) {
      pos += 3 + *(const ushort *)(clx + pos + 1);
    } else {
      return NULL;
    }
  }
  return NULL;
}

bool doc_text(const Storage &storage, FILE *text_file) {
  const uchar *wds;
  uint wds_size;
  if (!storage.stream("WordDocument", &wds, &wds_size)) {
    return NULL;
  }

  const uchar *pt = get_piece_table(storage);
  if (pt == NULL) {
    return false;
  }

  fputc(0xFE, text_file);
  fputc(0xFF, text_file);
  uint lcb = *(const uint *)(pt + 1);
  uint n = (lcb - 4) / 12;
  const uchar *pcd = pt + 5;
  for (uint i = 0; i < n; ++i) {
    uint start = *(const uint *)(pcd + i * 4);
    uint end = *(const uint *)(pcd + i * 4 + 4);
    if (start >= end) return false;
    uint fcCompressed = *(const uint *)(pcd + (n + 1) * 4 + i * 8 + 2);
    bool fCompressed = (fcCompressed >> 30) & 1;
    if (fCompressed) {
      uint fc = (fcCompressed & ~(1U << 30)) / 2;
      uint cb = end - start;
      for (uint i = 0; i < cb; ++i) {
        if (fc + i < wds_size) {
          fputc(wds[fc + i], text_file);
          fputc(0, text_file);
        }
      }
    } else {
      uint fc = fcCompressed;
      uint cb = (end - start) * 2;
      for (uint i = 0; i < cb; ++i) {
        if (fc + i < wds_size) {
          fputc(wds[fc + i], text_file);
        }
      }
    }
  }
  return true;
}
