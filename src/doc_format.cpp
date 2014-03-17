#include "doc_format.hpp"
#include <cstdio>
#include <set>
#include <algorithm>
using namespace std;

bool doc_format(const Storage &st) {
  const uchar *wds, *ts;
  uint wds_size, ts_size;

  // 取得WordDocument Stream (wds)
  if (!st.stream("WordDocument", &wds, &wds_size)) {
    return false;
  }

  // 取得Table Stream (ts)
  ushort flag1 = *(ushort *)((char *)wds + 0xA);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  if (!st.stream(is1Table ? "1Table" : "0Table", &ts, &ts_size)) {
    return false;
  }

  uint fcPlcfBtePapx = *(uint *)(wds + 258);
  uint lcbPlcfBtePapx = *(uint *)(wds + 258 + 4);
  uint n = (lcbPlcfBtePapx - 4) / 8;

  set< pair<uint, uint> > ranges;
  for (uint i = 0; i < n; ++i) {
    /* 读取PlcBtePapx中的信息（其实PlcBtePapx在Table Stream中） */
    int fc_start = *(int *)(ts + fcPlcfBtePapx + 4 * i);
    int fc_end = *(int *)(ts + fcPlcfBtePapx + 4 * i + 4);
    uint PnBtePapx = *(uint *)(ts + fcPlcfBtePapx + 4 * (n + 1 + i));

    const uchar *PapxFkp = wds + PnBtePapx * 512;
    uchar cpara = *(const uchar *)(PapxFkp + 511);

    for (uint i = 0; i < cpara; ++i) {
      // rgfc确定文本的位置
      uint rgfc_start = *(uint *)(PapxFkp + 4 * i);
      uint rgfc_end = *(uint *)(PapxFkp + 4 * i + 4);

      // bOffset找到PapxInFkp在PapxFkp中的位置
      uchar bOffset = *(uchar *)(PapxFkp + 4 * (cpara + 1) + 13 * i);

      // 找到PapxInFkp其中的cb, grpprlInPapx
      uchar cb = *(uchar *)(PapxFkp + bOffset * 2);

      // 得到GrpPrlAndIstd
      uint lcbGrpPrlAndIstd = 2 * cb - 1;
      const uchar *GrpPrlAndIstd = PapxFkp + bOffset * 2 + 1;
      if (cb == 0) {
        lcbGrpPrlAndIstd = *GrpPrlAndIstd * 2;
        GrpPrlAndIstd += 1;
      }

      // 根据GrpPrlAndIstd得到段落格式（Direct Paragraph Formatting）
      const uchar *grpprl = GrpPrlAndIstd + 2;
      uint lcb = lcbGrpPrlAndIstd - 1;
      static const int operand_size[8] = { 1, 1, 2, 4, 2, 2, -1, 3, };
      for (uint i = 0; i < lcb;) {
        ushort sprm = *(ushort *)(grpprl + i);
        uint spra = sprm / 8192;
        uint operand;
        i += 2;
        if (spra == 6) {
          i += 1 + *(uchar *)(grpprl + i);
        } else {
          if (operand_size[spra] == 1) {
            operand = *(uchar *)(grpprl + i);
          } else if (operand_size[spra] == 2) {
            operand = *(ushort *)(grpprl + i);
          } else if (operand_size[spra] == 3) {
            operand = *(uint *)(grpprl + i) & 0x00FFFFFF;
          } else if (operand_size[spra] == 4) {
            operand = *(uint *)(grpprl + i);
          } else {
            operand = -1;
          }
          i += operand_size[spra];
        }
        if ((sprm == 0x2403 || sprm == 0x2416) && operand == 1) { // 居中
          ranges.insert(make_pair(rgfc_start, rgfc_end));
        }
      }
    }
  }

  /* 合并各个区间 */
  set< pair<uint, uint> >::iterator iter = ranges.begin(), jter, kter;
  while (iter != ranges.end()) {
    jter = iter;
    ++jter;
    while (jter != ranges.end() && iter->second == jter->first) {
      pair<uint, uint> temp = *iter;
      temp.second = jter->second;
      ranges.erase(iter);
      ranges.insert(temp);
      iter = ranges.find(temp);
      kter = jter;
      ++jter;
      ranges.erase(kter);
    }
    iter = jter;
  }

  /*
  FILE *temp_file = fopen("/dev/shm/middle.txt", "wb");
  for (iter = ranges.begin(); iter != ranges.end(); ++iter) {
    printf("range[%u,%u] is in middle\n", iter->first, iter->second);
    for (uint i = iter->first; i < iter->second && i < wds_size; ++i) {
      fputc(wds[i], temp_file);
    }
  }
  fclose(temp_file);
  */
  return true;
}
