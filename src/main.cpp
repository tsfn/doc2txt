#include "ole.h"

using tsfn::get16;
using tsfn::get32;

// in Unicode(UTF-16LE)
bool WriteStream(FILE *file, const std::vector<uint8_t> &str) {
  if (file == NULL) {
    return false;
  }
  // BOM: Little-Endian
  fputc(0xFF, file);
  fputc(0xFE, file);
  for (std::vector<uint8_t>::const_iterator iter = str.begin(); iter != str.end(); ++iter) {
    if (fputc(*iter, file) == EOF) {
      return false;
    }
  }
  return true;
}


/*
 * 从分析好了的Storage中取得文本
 */
bool retrieve_text(const Storage &st, __out std::vector<uint8_t> &text) {
  std::vector<uint8_t> wds;
  if (!st.read_stream("WordDocument", wds)) {
    return false;
  }

  // WordDocument Stream {
  //  FIB {
  //    32bytes FibBase; {
  //      2bytes wIdent;
  //      2bytes nFib;
  //      ...
  //      1bit G-fWhichTblStm; // 标志所用的Table Stream是0Table还是1Table
  //      ...
  //    }
  //    2bytes csw;
  //    28bytes fibRgW;
  //    2bytes cslw;
  //    88bytes fibRgLw;
  //    2bytes cbRgFcLcb; //指明fibRgFcLcb的版本
  //    Variable fibRgFcLcb; // (fibRgFcLcbBlob，FIB中第一个变长的结构) {
  //      744bytes fibRgFcLcb97; {
  //        ...
  //        4bytes fcClx; // WDS[0x01A2] Clx在Table Stream中的偏移位置
  //        4bytes lcbClx; // WDS[0x01A6] Clx的长度
  //                // fcClx和lcbClx在WordDocument Stream中的相对位置是固定的
  //        ...
  //      }
  //      ...
  //    }
  //    ...
  //  }
  // }

  // 取得Table Stream
  std::vector<uint8_t> piece_stream;
  uint16_t flag1 = get16(wds, 0x000A);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  if (!st.read_stream(is1Table ? "1Table" : "0Table", piece_stream)) {
    return false;
  }

  // 从Table Stream中取得Clx结构
  uint32_t fcClx = get32(wds, 0x01A2);
  uint32_t lcbClx = get32(wds, 0x01A6);
  std::vector<uint8_t> clx;
  for (uint32_t i = 0; i < lcbClx; ++i) {
    if (fcClx + i >= piece_stream.size()) {
      return false;
    }
    clx.push_back(piece_stream[fcClx + i]);
  }

  // Clx {
  //  Variable Prc[] { // 若干个Prc的数组
  //    1byte clxt; // 一定是0x01
  //    Variable PrcData {
  //      2bytes cbGrpprl; // 指明GrpPrl的大小，单位为字节
  //      Variable GrpPrl;
  //    }
  //  }
  //  Variable Pcdt {
  //    1byte clxt; // 一定是0x02
  //    4bytes lcb; // PlcPcd的长度
  //    Variable PlcPcd { // 用来描述所有文本块的两个数组
  //      Variable CP[n+1]; // n+1个CP
  //      Variable Pcd[n]; {// n个Pcd
  //        1bit fNoParaLast; // 如果是1，文本块一定没有段标记
  //        1bit fR1; // 未定义
  //        1bit fDirty; // 一定是0
  //        13bits fR2; // 未定义
  //        4bytes fc; // 文本块在WordDocument Stream中的位置
  //        2bytes prm; // 用来描述直接段格式和直接字格式
  //      }
  //    }
  //  }
  // }

  // 从Clx中取得Piece Table
  std::vector<uint8_t> piece_table;//Piece Table
  for (int pos = 0;;) {
    uint8_t type = clx[pos];
    if (type == 2) { // Prc
      int len = get32(clx, pos + 1);
      piece_table.clear();
      for (int i = 0; i < len; ++i) {
        piece_table.push_back(clx[pos + 5 + i]);
      }
      break;
    } else if (type == 1) { // Pcdt
      pos += 3 + get16(clx, pos + 1);
    } else {
      return false;
    }
  }

  // 根据Piece Table从WordDocument Stream中取得各个文本块
  text.clear();
  int lcbPieceTable = piece_table.size();
  int piece_count = (lcbPieceTable - 4) / 12;
  uint16_t map[256] = {}; // map[uchar] = UTF_16_char
  map[0x82] = 0x201A;
  map[0x83] = 0x0192;
  map[0x84] = 0x201E;
  map[0x85] = 0x2026;
  map[0x86] = 0x2020;
  map[0x87] = 0x2021;
  map[0x88] = 0x02C6;
  map[0x89] = 0x2030;
  map[0x8A] = 0x0160;
  map[0x8B] = 0x2039;
  map[0x8C] = 0x0152;
  map[0x91] = 0x2018;
  map[0x92] = 0x2019;
  map[0x93] = 0x201C;
  map[0x94] = 0x201D;
  map[0x95] = 0x2022;
  map[0x96] = 0x2013;
  map[0x97] = 0x2014;
  map[0x98] = 0x02DC;
  map[0x99] = 0x2122;
  map[0x9A] = 0x0161;
  map[0x9B] = 0x203A;
  map[0x9C] = 0x0153;
  map[0x9F] = 0x0178;
  for (int i = 0; i < piece_count; ++i) {
    uint32_t cp_start = get32(piece_table, i * 4);
    uint32_t cp_end = get32(piece_table, (i + 1) * 4);
    if (cp_start >= cp_end) {
      return false;
    }

    // 32bits fcCompressed {
    //  // 如果fCompressed=0，文本块是一个从fc开始的16-bit的Unicode的字符数组
    //  // 如果fCompressed=1，文本块是一个从fc/2开始的8-bit的Unicode的字符数组
    //      除了map[]中列出来的例外
    //  30bits fc {
    //  }
    //  [30] bit fCompressed // 表示当前文本块是否被压缩过
    //  [31] bit r1; // 一定是0
    // }
    uint32_t fcCompressed = get32(piece_table, (piece_count + 1) * 4 + i * 8 + 2);
    if (((fcCompressed >> 31) & 1) != 0) {
      return false;
    }

    bool fCompressed = (fcCompressed >> 30) & 1;
    if (fCompressed == 0) {
      uint32_t fc = fcCompressed;
      uint32_t cb = (cp_end - cp_start) * 2;
      for (uint32_t j = 0; j < cb; ++j) {
        text.push_back(wds[fc + j]);
      }
    } else {
      uint32_t fc = (fc & ~(1U << 30)) / 2;
      uint32_t cb = cp_end - cp_start;
      for (uint32_t j = 0; j < cb; ++j) {
        uint8_t ch = wds[fc + j];
        if (map[ch] != 0) {
          text.push_back(map[ch] & ((1U << 8) - 1));
          text.push_back(map[ch] >> 8);
        } else {
          text.push_back(ch);
          text.push_back(0);
        }
      }
    }
  }
  return true;
}

/*
 * 以缩减的形式列出doc文件中的结构
 */
void list_directory_tree(const Storage &s, int DirID, int white_space) {
  for (int i = 0; i < white_space; ++i) {
    printf(" ");
  }

  DirEntry dir_entry;
  if (!s.get_dir_entry(DirID, dir_entry)) {
    printf("get DirID(%d)'s dir_entry failed!\n", DirID);
    return;
  }

  char name[32];
  for (int i = 0; i < 32; ++i) {
    name[i] = dir_entry._ab[2 * i];
  }
  printf("(DirID = %d) %s, size = %u ", DirID, name, dir_entry._ulSize);

  std::vector<uint32_t> children;
  if (s.entry_children(DirID, children)) {
    printf("(%d children)\n", children.size());
    for (unsigned i = 0; i < children.size(); ++i) {
      list_directory_tree(s, children[i], white_space + 2);
    }
  } else {
    printf("get _dir[%d]'s children failed!\n", DirID);
  }
}


int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "at least 2 files: arg[1]=input file, arg[2]=output file\n");
    return 1;
  }

  // 打开输入文件
  FILE *file = fopen(argv[1], "rb");
  Storage s;
  if (!s.init(file)) {
    fclose(file);
    fprintf(stderr, "init fail!\n");
    return -1;
  }
  fclose(file);

  // 输入文件中有哪些流
  s.__print_dir();

  // 输入文件中中各个流的之间的层次结构
  list_directory_tree(s, 0, 0);

  // 取得文本，以UTF-16保存
  std::vector<uint8_t> str;
  if (!retrieve_text(s, str)) {
    fprintf(stderr, "init fail!\n");
    return -1;
  }

  // 输出文本
  FILE *tmp_file = fopen(argv[2], "wb");
  if (!WriteStream(tmp_file, str)) {
    return -1;
  }
  fclose(tmp_file);

  return 0;
}
