#include "doc_text.hpp"
#include <cstdlib>

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
  uint lcbClx = *(const uint *)(wds + 0x01A6);
  const uchar *clx = ts + fcClx;
  for (uint pos = 0; clx + pos < ts + ts_size && pos < lcbClx;) {
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

/*
 * 提取文本块，不处理其中的非法字符，返回一个指向文本段的指针
 */
uchar *doc_text(const Storage &storage) {
  const uchar *wds;
  uint wds_size;
  if (!storage.stream("WordDocument", &wds, &wds_size)) {
    return NULL;
  }

  const uchar *pt = get_piece_table(storage);
  if (pt == NULL) {
    return NULL;
  }

  uint buf_size = 0; // 文本块的大小

  uint lcb = *(const uint *)(pt + 1);
  uint n = (lcb - 4) / 12;
  const uchar *pcd = pt + 5;
  for (uint i = 0; i < n; ++i) {
    uint start = *(const uint *)(pcd + i * 4);
    uint end = *(const uint *)(pcd + i * 4 + 4);
    if (start >= end) return NULL;
    uint fcCompressed = *(const uint *)(pcd + (n + 1) * 4 + i * 8 + 2);
    bool fCompressed = (fcCompressed >> 30) & 1;
    if (fCompressed) {
      uint cb = end - start;
      buf_size += cb * 2;
    } else {
      uint cb = (end - start) * 2;
      buf_size += cb;
    }
  }

  uchar *buf = (uchar *)malloc(buf_size + 4);
  uint ptr = 0;
  if (buf == NULL) return NULL;

  for (uint i = 0; i < n; ++i) {
    uint start = *(const uint *)(pcd + i * 4);
    uint end = *(const uint *)(pcd + i * 4 + 4);
    uint fcCompressed = *(const uint *)(pcd + (n + 1) * 4 + i * 8 + 2);
    bool fCompressed = (fcCompressed >> 30) & 1;
    if (fCompressed) {
      uint fc = (fcCompressed & ~(1U << 30)) / 2;
      uint cb = end - start;
      for (uint i = 0; i < cb; ++i) {
        if (fc + i < wds_size) {
          buf[ptr++] = wds[fc + i];
          buf[ptr++] = 0;
        }
      }
    } else {
      uint fc = fcCompressed;
      uint cb = (end - start) * 2;
      for (uint i = 0; i < cb; ++i) {
        buf[ptr++] = wds[fc + i];
      }
    }
  }
  return buf;
}

/*
 * 提取文本块，到指定文件中
 */
bool doc_text_file(const Storage &storage, FILE *text_file) {
  const uchar *wds;
  uint wds_size;
  if (!storage.stream("WordDocument", &wds, &wds_size)) {
    return false;
  }

  const uchar *pt = get_piece_table(storage);
  if (pt == NULL) {
    return false;
  }

  fputc(0xFF, text_file);
  fputc(0xFE, text_file);
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
      for (uint i = 0; i < cb; i += 2) {
        if (fc + i + 1 < wds_size) {
          uchar low = wds[fc + i], high = wds[fc + i + 1];
          ushort ch = ((ushort)high << 8) + low;
          if (ch >= 0x000A) {
            fputc(wds[fc + i], text_file);
            fputc(wds[fc + i + 1], text_file);
          }
        }
      }
    }
  }
  return true;
}
