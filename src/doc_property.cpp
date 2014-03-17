#include "doc_property.hpp"
#include "Storage.hpp"
#include <cstdio>
#include <cstdlib>

bool parse_summary_information(const Storage &st) {
  // 取得\005SummaryInformation Stream
  char sis_name[64];
  sis_name[0] = 5, sis_name[1] = 0;
  sprintf(sis_name + 2, "SummaryInformation");
  const uchar *sis;
  uint sis_size;
  if (!st.stream(sis_name, &sis, &sis_size)) {
    return false;
  }


  // 读取\005SummaryInformation Stream
  /*
  for (int i = 0; i < 324; ++i) {
    printf("[%04X] = %02X\n", i, sis[i]);
  }
  */

  /* \005SummaryInformation Stream是一个PropertySetStream
   * PropertySetStream {
   *   ByteOrder: (2 bytes);
   *   Version: (2 bytes);
   *   SystemIdentifier: (4 bytes);
   *   CLSID: (16 bytes);
   *   NumPropertySets: (4 bytes);
   *   FMTIDO: (16 bytes);
   *   Offset0: (4 bytes);
   *   FMTID1: (0 bytes);
   *   Offset1: (0 bytes);
   *   PropertySet0: PropertySet (variable);
   *   PropertySet1: PropertySet (variable);
   *   Padding (variable);
   * }
   * PropertySet {
   *   Size: (4 bytes); // 这个结构的中大小
   *   NumProperties: (4 bytes); // 这个结构有多少个Property
   *   PropertyIdentifierAndOffset[] {
   *     PropertyIdentifier: (4 bytes); // Property编号
   *     Offset: (4 bytes); // 从PropertySet的起始位置开始的偏移量
   *   }
   *   Property[] {
   *     TypedPropertyValue {
   *       Type: PropertyType (2 bytes);
   *       Padding: (2 bytes);
   *       Value: (variable) {
   *         Type=0x0008: CodePageString;
   *         Type=0x0040: FILETIME (Packet Version);
   *       }
   *     }
   *   }
   * }
   */

  FILE *out_file = fopen("./property.txt", "wb");

  const uchar *ps = sis + 48;
  uint pre_size = 0;
  uint num_property_set = *(uint *)(sis + 24);
  for (uint set_id = 0; set_id < num_property_set; ++set_id) {
    ps += pre_size;
    pre_size = *(uint *)(ps);
    uint num_properties = *(uint *)(ps + 4);
    printf("num_properties = %u\n", num_properties);
    for (uint i = 0; i < num_properties; ++i) {
      uint pid = *(uint *)(ps + 8 + 8 * i);
      uint offset = *(uint *)(ps + 8 + 8 * i + 4);
      ushort type = *(ushort *)(ps + offset);
      ushort padding = *(ushort *)(ps + offset + 2);
      printf("pid = %u, offset = %u, type = 0x%04X, padding = %d\n",
          pid, offset, type, padding);
      const uchar *value = ps + offset + 4;
      if (type == 0x001E) {
        uint size = *(uint *)(value);
        printf("size = %u\n", size);
        for (uint i = 0; i < size; ++i) {
          fputc(value[4 + i], out_file);
          // printf("value[0x%02X] = 0x%02X\n", 4 + i, value[4 + i]);
          // fputc(value[4 + i], file);
        }
      }
    }
  }
  return true;
}

bool doc_property(const Storage &st) {
  return parse_summary_information(st);
}
