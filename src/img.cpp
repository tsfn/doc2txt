#include "ole.hpp"

// 提取一个OfficeArtBStoreContainerFileBlock中的图片
bool storeBlipBlock(uchar *block, const char *img_name, uint size) {
  // uchar recVer = *(uchar *)(block) & 0xF;
  ushort recInstance = *(ushort *)(block) >> 4;
  ushort recType = *(ushort *)(block + 2);
  // ushort recLen = *(uint *)(block + 4);

  char str[128];
  uchar *start = block;
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
    return false;
  }

  FILE *file = fopen(str, "wb");
  for (uint i = 0; i < size; ++i) {
    fputc(start[i], file);
  }
  fclose(file);
  return true;
}

bool retrieve_image(const Storage &st, const char *file_path) {
  // 取得WordDocument Stream (wds)
  uchar *wds = c_read_stream(&st, "WordDocument");
  if (wds == NULL) {
    return false;
  }

  // 取得Table Stream (ts)
  uint16_t flag1 = *(ushort *)((char *)wds + 0xA);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  uchar *ts = c_read_stream(&st, is1Table ? "1Table" : "0Table");
  if (ts == NULL) {
    return false;
  }

  // fcDggInfo = 154 + (194 - 94) * 4
  uint fcDggInfo = *(uint *)(wds + 154 + (194 - 94) * 4);
  uint lcbDggInfo = *(uint *)(wds + 154 + (194 - 94) * 4 + 4);
  printf("fcDggInfo = %u, lcbDggInfo = %u\n", fcDggInfo, lcbDggInfo);
  if (lcbDggInfo == 0) {
    printf("\"If lcbDggInfo is zero, there MUST NOT be any drawings in the document.\"\n");
    return false;
  }

  uchar *OfficeArtContent = ts + fcDggInfo;

  /*
  uchar recVer = *(uchar *)(OfficeArtContent) & 0xF;
  ushort recInstance = *(ushort *)(OfficeArtContent) >> 4;
  ushort recType = *(ushort *)(OfficeArtContent + 2);
  ushort recLen = *(uint *)(OfficeArtContent + 4);
  printf("%d %d %d %d\n", recVer, recInstance, recType, recLen);
  */

  uchar *blipStore = OfficeArtContent + 16 + *(uint *)(OfficeArtContent + 12);
  /*
  printf("blipStore.rh.recType = 0x%02X\n", *(ushort *)(blipStore + 2));
  printf("blipStore.rh.recLen = %u\n", *(uint *)(blipStore + 4));
  */

  uchar *rgfb = blipStore + 8;
  int counter = 0;
  while (rgfb < blipStore + *(uint *)(blipStore + 4)) {
    char img_name[128];
    sprintf(img_name, "%s/img_%02d", file_path, ++counter);

    ushort recType = *(ushort *)(rgfb + 2);
    uint recSize = *(uint *)(rgfb + 4);
    if (recType == 0xF007) {
      uint size = *(uint *)(rgfb + 28);
      uint foDelay = *(uint *)(rgfb + 36);
      storeBlipBlock(wds + foDelay, img_name, size);
    } else {
      storeBlipBlock(rgfb, img_name, recSize);
    }

    rgfb += 8 + *(uint *)(rgfb + 4);
  }

  free(wds);
  free(ts);
  return true;
}
