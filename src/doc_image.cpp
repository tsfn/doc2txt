#include "doc_image.hpp"
#include <cstring>
#include <cstdlib>

// 提取一个OfficeArtBStoreContainerFileBlock中的图片
static bool storeBlipBlock(const uchar *block, const char *img_name, uint size) {
  // uchar recVer = *(uchar *)(block) & 0xF;
  ushort recInstance = *(ushort *)(block) >> 4;
  ushort recType = *(ushort *)(block + 2);
  // ushort recLen = *(uint *)(block + 4);

  char str[128];
  const uchar *start = block;
  if (recType == 0xF01A) { // EMF
    sprintf(str, "%s.emf", img_name);
    start += 8 + 16 + (recInstance == 0x3D5 ? 16 : 0) + 34;
  } else if (recType == 0xF01B) { // WMF
    sprintf(str, "%s.wmf", img_name);
    start += 8 + 16 + (recInstance == 0x217 ? 16 : 0) + 34;
  } else if (recType == 0xF01C) { // PICT
    sprintf(str, "%s.pict", img_name);
    start += 8 + 16 + (recInstance == 0x543 ? 16 : 0) + 34;
  } else if (recType == 0xF01D || recType == 0xF02A) { // JPEG
    sprintf(str, "%s.jpeg", img_name);
    start += (recInstance == 0x46B || recInstance == 0x6E3 ? 41 : 25);
  } else if (recType == 0xF01E) { // PNG
    sprintf(str, "%s.png", img_name);
    start += 8 + 16 + (recInstance == 0x6E1 ? 16 : 0) + 1;
  } else if (recType == 0xF01F) { // DIB
    sprintf(str, "%s.dib", img_name);
    start += 8 + 16 + (recInstance == 0x7A9 ? 16 : 0) + 1;
  } else if (recType == 0xF029) { // TIFF
    sprintf(str, "%s.tiff", img_name);
    start += 8 + 16 + (recInstance == 0x6E5 ? 16 : 0) + 1;
  } else {
    fprintf(stderr, "unknown recType: %u\n", (uint)recType);
    return false;
  }

  FILE *file = fopen(str, "wb");
  for (uint i = 0; i < size; ++i) {
    fputc(start[i], file);
  }
  fclose(file);
  return true;
}

static bool retrieve_image(const Storage &st, const char *file_path) {
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

  uint fcDggInfo = *(const uint *)(wds + 154 + (194 - 94) * 4);
  uint lcbDggInfo = *(const uint *)(wds + 154 + (194 - 94) * 4 + 4);
  if (lcbDggInfo == 0 || fcDggInfo >= ts_size) {
    return false;
  }

  const uchar *OfficeArtContent = ts + fcDggInfo;
  const uchar *blipStore = OfficeArtContent + 16 + *(const uint *)(OfficeArtContent + 12);
  uint lcbBlipStore = *(uint *)(blipStore + 4);
  // printf("lcbBlipStore = %u\n", lcbBlipStore);

  const uchar *rgfb = blipStore + 8;
  char *img_name = (char *)malloc(strlen(file_path) + 16);
  while (rgfb < blipStore + lcbBlipStore) {
    static int counter = 0;
    sprintf(img_name, "%s/%05d", file_path, ++counter);
    ushort recType = *(const ushort *)(rgfb + 2);
    uint recSize = *(const uint *)(rgfb + 4);
    /*
    printf("recType = %04X\n", recType);
    printf("recSize = %u\n", recSize);
    */
    if (recType == 0xF007) {
      uint size = *(const uint *)(rgfb + 28);
      uint foDelay = *(const uint *)(rgfb + 36);
      // printf("size = %u, foDelay = %u\n", size, foDelay);
      if (foDelay < wds_size && foDelay + size <= wds_size) {
        storeBlipBlock(wds + foDelay, img_name, size);
      }
    } else if (0xF018 <= recType && recType <= 0xF117) {
      storeBlipBlock(rgfb, img_name, recSize);
    } else {
      break;
    }
    rgfb += 8 + *(const uint *)(rgfb + 4);
  }
  free(img_name);
  return true;
}

static bool inline_image(const Storage &st, const char *file_path) {
  const uchar *wds, *ts, *ds;
  uint wds_size, ts_size, ds_size;

  // 取得Data Stream (ds)
  if (!st.stream("Data", &ds, &ds_size)) {
    return false;
  }

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


  uint fcPlcfBteChpx = *(uint *)(wds + 250);
  uint lcbPlcfBteChpx = *(uint *)(wds + 250 + 4);
  int n = (lcbPlcfBteChpx - 4) / 8;

  for (int i = 0; i < n; ++i) {
    uint PnBteChpx = *(uint *)(ts + fcPlcfBteChpx + 4 * (n + 1 + i));
    const uchar *ChpxFkp = wds + PnBteChpx * 512;
    uchar crun = *(const uchar *)(ChpxFkp + 511);

    for (uint i = 0; i < crun; ++i) {
      // rgfc确定文本的位置
      uint rgfc_start = *(const uint *)(ChpxFkp + 4 * i);
      ushort ch = *(const ushort *)(wds + rgfc_start);
      if (ch != 0x0001) {
        continue;
      }

      // bOffset找到ChpxInFkp在ChpxFkp中的位置
      uchar bOffset = *(const uchar *)(ChpxFkp + 4 * (crun + 1) + 1 * i);
      if (bOffset == 0) {
        continue;
      }

      // 找到ChpxInFkp其中的lcb, grpprlInChpx
      uchar lcb = *(const uchar *)(ChpxFkp + bOffset * 2);
      const uchar *grpprl = ChpxFkp + 2 * bOffset + 1;
      static const int operand_size[8] = { 1, 1, 2, 4, 2, 2, -1, 3, };
      for (uint i = 0; i < lcb;) {
        ushort sprm = *(ushort *)(grpprl + i);
        uint spra = sprm / 8192;
        uint operand;
        i += 2;
        if (spra == 6) {
          i += 1 + *(const uchar *)(grpprl + i);
        } else {
          if (operand_size[spra] == 1) {
            operand = *(const uchar *)(grpprl + i);
          } else if (operand_size[spra] == 2) {
            operand = *(const ushort *)(grpprl + i);
          } else if (operand_size[spra] == 3) {
            operand = *(const uint *)(grpprl + i) & 0x00FFFFFF;
          } else if (operand_size[spra] == 4) {
            operand = *(const uint *)(grpprl + i);
          } else {
            operand = -1;
          }
          i += operand_size[spra];
        }

        if (sprm != 0x6A03) {
          continue;
        }

        const uchar *PICF = ds + operand;
        uint lcbPICF = *(const uint *)PICF;

        ushort picf_mfpf_mm = *(const ushort *)(PICF + 6);
        const uchar *picture = PICF + 68 + (picf_mfpf_mm == 0x0066 ? 1 + *(PICF + 68) : 0);
        const uchar *rgfb = picture + 8 + *(const uint *)(picture + 4);

        char *img_name = (char *)malloc(strlen(file_path) + 16);
        while (PICF <= rgfb && rgfb < PICF + lcbPICF) {
          static int counter = 0;
          sprintf(img_name, "%s/i%05d", file_path, ++counter);
          uint recSize = *(const uint *)(rgfb + 4);
          uchar cbName = *(rgfb + 41);
          storeBlipBlock(rgfb + 44, img_name, recSize - cbName - 36);
          rgfb += 8 + recSize;
        }
        free(img_name);
      }
    }
  }
  return true;
}

bool doc_image(const Storage &storage, const char *image_dir_path) {
  retrieve_image(storage, image_dir_path);
  inline_image(storage, image_dir_path);
  return true;
}
