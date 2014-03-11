#include "ole.hpp"

using tsfn::get16;
using tsfn::get32;

// WriteStream in UTF-16LE {{{
// in Unicode(UTF-16LE)
bool WriteStream(FILE *file, const std::vector<uint8_t> &str) {
  if (file == NULL) {
    return false;
  }

  // BOM: Little-Endian
  fputc(0xFF, file);
  fputc(0xFE, file);

  /*
  for (uint i = 0; i < str.size(); i += 2) {
    printf("0x%02X%02X ", str[i + 1], str[i]);
  }
  printf("\n");
  */
  for (std::vector<uint8_t>::const_iterator iter = str.begin(); iter != str.end(); ++iter) {
    if (fputc(*iter, file) == EOF) {
      return false;
    }
  }
  return true;
}
// }}}

// to_string(ushort ch) {{{
std::string to_string(ushort ch) {
  std::string str;
  if (ch == 0x0020) {
    // return " ";
    return "空格";
  } else if (ch == 0x000D) {
    // return "\n";
    return "回车";
  } else if (ch == 0x0007) {
    // return "\t";
    return "制表符Tab";
  } else

  if (33 <= ch && ch < 128) {
    str += (char)ch;
  } else {
    char buf[20];
    sprintf(buf, "0x%04X", (int)ch);
    str = std::string(buf);
  }
  return str;
}
// }}}

// get piece_table {{{
// pcd=piece_table
bool get_pcd(const Storage &st, __out std::vector<uint8_t> &piece_table) {
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
  piece_table.clear();
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
  return true;
}
// }}}

// retrieving text {{{
/*
 * 从分析好了的Storage中取得文本
 */
bool retrieve_text(const Storage &st, __out std::vector<uint8_t> &text) {
  // 取得Piece Table
  std::vector<uint8_t> piece_table;//Piece Table
  if (!get_pcd(st, piece_table)) {
    return false;
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

  std::vector<uint8_t> wds;
  if (!st.read_stream("WordDocument", wds)) {
    return false;
  }

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

    fprintf(stderr, "cp_start = 0x%X, cp_end = 0x%X, fcCompressed = 0x%X\n",
        cp_start, cp_end, fcCompressed);

    bool fCompressed = (fcCompressed >> 30) & 1;
    if (fCompressed == 0) {
      uint32_t fc = fcCompressed;
      uint32_t cb = (cp_end - cp_start) * 2;
      for (uint32_t j = 0; j < cb; ++j) {
        text.push_back(wds[fc + j]);
      }
    } else {
      uint32_t fc = (fcCompressed & ~(1U << 30)) / 2;
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
// }}}

// list directories {{{
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

  printf("(DirID = %d) %s, size = %u, sectStart = %u ",
      DirID, dir_entry.__name().c_str(),
      dir_entry._ulSize, dir_entry._sectStart);

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
// }}}

std::set<uint> sprms;

// parse_format {{{
std::set<ushort> parse_grpprl(uchar *grpprl, uint lcb) {
  static const int operand_size[8] = { 1, 1, 2, 4, 2, 2, -1, 3, };
  std::set<ushort> sprms;

  uint i = 0;
  while (i < lcb) {
    ushort sprm = *(ushort *)(grpprl + i);
    // uint operand;

    // uint ispmd = sprm & 0x01FF;
    // uint f = (sprm / 512) & 0x0001;
    // uint sgc = (sprm / 1024) & 0x0007;
    uint spra = sprm / 8192;

    i += 2;
    if (spra == 6) {
      i += 1 + *(uchar *)(grpprl + i);
    } else {
      /*
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
      */
      i += operand_size[spra];
    }

    sprms.insert(sprm);
  }
  return sprms;
}

bool parse_format(const Storage &st, FILE *file) {
  uchar *wds = c_read_stream(&st, "WordDocument");
  uint16_t flag1 = *(ushort *)((char *)wds + 0xA);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  uchar *table_stream = c_read_stream(&st, is1Table ? "1Table" : "0Table");

  if (wds == NULL || table_stream == NULL || file == NULL) {
    return false;
  }

  uint fcPlcfBteChpx = *(uint *)(wds + 250);
  uint lcbPlcfBteChpx = *(uint *)(wds + 250 + 4);
  uint n = (lcbPlcfBteChpx - 4) / 8;
  fprintf(file, "<html>\n");
  for (uint i = 0; i < n; ++i) {
    uint PnBteChpx = *(uint *)(table_stream + fcPlcfBteChpx + 4 * (n + 1 + i));
    uchar *ChpxFkp = wds + PnBteChpx * 512;
    uchar crun = *(uchar *)(ChpxFkp + 511);
    for (ushort j = 0; j < crun; ++j) {
      uint rgfc_start = *(uint *)(ChpxFkp + 4 * j);
      uint rgfc_end = *(uint *)(ChpxFkp + 4 * j + 4);
      uchar bOffset = *(uchar *)(ChpxFkp + 4 * (crun + 1) + j);
      uchar cb = *(uchar *)(ChpxFkp + bOffset * 2);
      uchar *Chpx = ChpxFkp + 2 * bOffset;

      std::set<ushort> sprms = parse_grpprl(Chpx + 1, cb);
      for (auto sprm: sprms) {
        if (sprm == 0x0835) {
          fprintf(file, "<strong>");
        } else if (sprm == 0x0836) {
          fprintf(file, "<i>");
        } else if (sprm == 0x2A3E) {
          fprintf(file, "<u>");
        }
      }
      for (uint j = rgfc_start; j < rgfc_end; j += 2) {
        fprintf(file, "%s", to_string(*(ushort *)(wds + j)).c_str());
      }
      for (auto sprm: sprms) {
        if (sprm == 0x0835) {
          fprintf(file, "</strong>");
        } else if (sprm == 0x0836) {
          fprintf(file, "</i>");
        } else if (sprm == 0x2A3E) {
          fprintf(file, "</u>");
        }
      }
    }
  }
  fprintf(file, "</html>\n");

  fclose(file);
  free(wds);
  free(table_stream);
  return true;
}
// }}}

// read_grpprl {{{
bool read_grpprl(uchar *grpprl, uint lcb) {
  static const int operand_size[8] = {
    1, // spra=0 Operand is a ToggleOperand (1 byte)
    1, // spra=1
    2, // spra=2
    4, // spra=3
    2, // spra=4
    2, //spra=5
    -1, // spra=6, Operand is of variable length, 
        // the first byte of the operand is the size of the rest of the operand
    3, // spra=7
  };

  printf("lcb = %d:\n", lcb);
  /*
  for (uint i = 0; i < lcb; ++i) {
    printf("%02X ", *(grpprl + i));
  }
  printf("\n");
  */

  uint i = 0;
  while (i < lcb) {
    ushort sprm = *(ushort *)(grpprl + i);

    // uint ispmd = sprm & 0x01FF;
    // uint f = (sprm / 512) & 0x0001;
    uint sgc = (sprm / 1024) & 0x0007;
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

    sprms.insert(sprm);
    printf("sprm = 0x%04X, sprm.sgc=%d, operand = 0x%04X\n", sprm, sgc, operand);

    /*
     * 这一段的文本是居中的
     */
    if ((sprm == 0x2403/*物理位置上的居中*/ || sprm == 0x2416/*逻辑位置上的居中*/)
        && operand == 1) {
      return true;
    }

//    printf("=(sgc=%d,sprm=0x%04X)", sgc, sprm);
  }
  return false;
}
// }}}

// Direct Paragraph Formatting {{{
bool paragraph_formatting(const Storage &st)  {
  printf("==============Paragraph Formatting================\n");
  uchar *wds = c_read_stream(&st, "WordDocument");
  if (wds == NULL) {
    return false;
  }

  // 取得Table Stream
  uint16_t flag1 = *(ushort *)((char *)wds + 0xA);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  uchar *table_stream = c_read_stream(&st, is1Table ? "1Table" : "0Table");
  if (table_stream == NULL) {
    return false;
  }

  /*
   * Fib {
   *   FibBase;
   *   ...
   *   cbRgFcLcb;
   *   fibRgFcLcbBlob;
   *   ...
   * }
   * fibRgFcLcbBlob在Fib中的位置是(char *)wds + 154
   * fcPlcfBtePapx在FibRgFcLcb97中下标是26
   * 154 + 26 * 4 = 154 + 104 = 258
   */
  uint fcPlcfBtePapx = *(uint *)(wds + 258);
  uint lcbPlcfBtePapx = *(uint *)(wds + 258 + 4);
  int n = (lcbPlcfBtePapx - 4) / 8;

  /*
   * PlcBtePapx在Table Stream中
   * PlcBtePapx { // PLC structure
   *   4bytes*(n+1) aFC[] {
   *     // PlcBtePapx maps stream offsets to data
   *   }
   *   4bytes*n aPnBtePapx[] {
   *     4bytes PnFkpPapx {
   *       22bits pn; // specifies the offset in the WDS of a PapxFkp struct
   *       10bits unused;
   *     }
   *   }
   * }
   */

  std::vector<uint8_t> debug_info;

  for (int i = 0; i < n; ++i) {
    /* 读取PlcBtePapx中的信息（其实PlcBtePapx在Table Stream中） */
    int fc_start = *(int *)(table_stream + fcPlcfBtePapx + 4 * i);
    int fc_end = *(int *)(table_stream + fcPlcfBtePapx + 4 * i + 4);
    uint PnBtePapx = *(uint *)(table_stream + fcPlcfBtePapx + 4 * (n + 1 + i));
    printf("--> PlcBtePapx[%d] = (%04X...%04X): %04X\n",
        i, fc_start, fc_end, PnBtePapx);

    /* PapxFkp将段落等部分映射到相应的属性上去
     * PapxFkp (512bytes) {
     *   rgfc: uint[cpara+1] {
     *     rgfc[i]; // specifies an offset in WDS where a paragraph of text begin.
     *   }
     *   rgbx: BxPap[cpara] {
     *     // BxPap specify the offset of one of the PapxInFkp structures
     *     // in this PapxFkp structure
     *     BxPap (13bytes) {
     *       // bOffset*2 specifies the offset of a PapxInFkp in a PapxFkp
     *       bOffset (1byte);
     *       reserved (12bytes);
     *     }
     *   }
     *   PapxInFkp[..] {
     *     // if cb!=0, grpprlInPapx is 2*cb-1 bytes long
     *     // if cb==0, grpprlInPapx's size is specified by its first byte
     *     cb (1byte);
     *
     *     // if cb!=0, grpprlInPapx is GrpPrlAndIstd
     *     // if cb==0, cb'*2 is the size of bytes of the rest of grpprlInPapx,
     *     //   The bytes after cb' form a GrpPrlAndIstd. 
     *     grpprlInPapx (variable) {
     *       cb' (1byte, optional);
     *       GrpPrlAndIstd (variable) {
     *         // 解释在下面
     *       }
     *     }
     *   }
     *   ..
     *   cpara(last 1byte); // 0x01 <= cpara <= 0x1D
     * }
     */

    uchar *PapxFkp = wds + PnBtePapx * 512;
    uchar cpara = *(uchar *)(PapxFkp + 511);
    fprintf(stdout, "cpara = 0x%02X\n", (int)cpara);

    for (size_t i = 0; i < cpara; ++i) {
      // rgfc确定文本的位置
      uint rgfc_start = *(uint *)(PapxFkp + 4 * i);
      uint rgfc_end = *(uint *)(PapxFkp + 4 * i + 4);
      printf("[rgfc_start = 0x%X, rgfc_end = 0x%X]\n", rgfc_start, rgfc_end);
      for (uint cp = rgfc_start; cp != rgfc_end; cp += 2) {
        printf("    <0x%04X> = %s\n", cp, to_string(*(ushort *)(wds + cp)).c_str());
      }

      // bOffset找到PapxInFkp在PapxFkp中的位置
      uchar bOffset = *(uchar *)(PapxFkp + 4 * (cpara + 1) + 13 * i);

      // 找到PapxInFkp其中的cb, grpprlInPapx
      uchar cb = *(uchar *)(PapxFkp + bOffset * 2);
      // printf("bOffset*2 = %d, cb=%d, cd'=%d\n", (int)bOffset * 2, cb, *(PapxFkp + bOffset*2+1));

      /*
       * GrpPrlAndIstd (variable) {
       *   // istd: an integer that specifies the style that is applied to
       *   // this paragraph, cell marker or table row marker,
       *   // refer [Applying Properties] to interpret this value.
       *   istd (2bytes);
       *
       *   // grpprl: specifies the properties of this paragraph etc.
       *   grpprl: Prl[] (variable) {
       *     Prl (variable) {
       *       sprm (2 bytes); // property to be modified
       *       operand (sprm.spra bytes); // its meaning depends on sprm
       *     }
       *   }
       * }
       */

      // 得到GrpPrlAndIstd
      uint lcbGrpPrlAndIstd = 2 * cb - 1;
      uchar *GrpPrlAndIstd = PapxFkp + bOffset * 2 + 1;
      if (cb == 0) {
        lcbGrpPrlAndIstd = *GrpPrlAndIstd * 2;
        GrpPrlAndIstd += 1;
      }
      // printf("lcbGrpPrlAndIstd = %u, GrpPrlAndIstd = %u\n", lcbGrpPrlAndIstd, (uint)GrpPrlAndIstd);

      printf("istd = 0x%04X\n", *GrpPrlAndIstd);
      // 根据GrpPrlAndIstd得到段落格式（Direct Paragraph Formatting）
      if (read_grpprl(GrpPrlAndIstd + 2, lcbGrpPrlAndIstd - 2) || true) {
        for (uint i = rgfc_start; i < rgfc_end; ++i) {
          debug_info.push_back(wds[i]);
        }
      }
    }
  }

  FILE *debug_file = fopen("/dev/shm/debug_file.txt", "wb");
  WriteStream(debug_file, debug_info);
  fclose(debug_file);

  // 取得Piece Table
  std::vector<uint8_t> pcd;//Piece Table
  if (!get_pcd(st, pcd)) {
    return false;
  }

  free(wds);
  free(table_stream);
  return true;
}
// }}}

// Direct Character Formatting {{{
bool character_formatting(const Storage &st)  {
  printf("==============Charactoer Formatting================\n");
  uchar *wds = c_read_stream(&st, "WordDocument");
  if (wds == NULL) {
    return false;
  }

  // 取得Table Stream
  uint16_t flag1 = *(ushort *)((char *)wds + 0xA);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  uchar *table_stream = c_read_stream(&st, is1Table ? "1Table" : "0Table");
  if (table_stream == NULL) {
    return false;
  }

  /*
   * Fib {
   *   FibBase;
   *   ...
   *   cbRgFcLcb;
   *   fibRgFcLcbBlob;
   *   ...
   * }
   * fibRgFcLcbBlob在Fib中的位置是(char *)wds + 154
   * fcPlcfBteChpx在FibRgFcLcb97中下标是24
   * 154 + 24 * 4 = 154 + 96 = 250
   */
  uint fcPlcfBteChpx = *(uint *)(wds + 250);
  uint lcbPlcfBteChpx = *(uint *)(wds + 250 + 4);
  int n = (lcbPlcfBteChpx - 4) / 8;

  /*
   * PlcBteChpx在Table Stream中
   * PlcBteChpx { // PLC structure
   *   4bytes*(n+1) aFC[] {
   *     // PlcBteChpx maps stream offsets to data
   *   }
   *   4bytes*n aPnBteChpx[] {
   *     4bytes PnFkpChpx {
   *       22bits pn; // specifies the offset in the WDS of a ChpxFkp struct
   *       10bits unused;
   *     }
   *   }
   * }
   */

  std::vector<uint8_t> debug_info;

  for (int i = 0; i < n; ++i) {
    /*
     * 从TableStream中读取关于PlcBteChpx的信息
     */
    int fc_start = *(int *)(table_stream + fcPlcfBteChpx + 4 * i);
    int fc_end = *(int *)(table_stream + fcPlcfBteChpx + 4 * i + 4);
    uint PnBteChpx = *(uint *)(table_stream + fcPlcfBteChpx + 4 * (n + 1 + i));
    printf("--> PlcBteChpx[%d] = (%04X...%04X): %04X\n",
        i, fc_start, fc_end, PnBteChpx);

    /*
     * ChpxFkp将文字等部分映射到相应的属性上去
     * ChpxFkp (512bytes) {
     *   rgfc: uint[crun+1] {
     *     rgfc[i]; // WordDocument Stream中的偏移量
     *   }
     *   rgb: BxPap[crun] {
     *     // bOffset*2指定了一个Chpx在ChpxFkp中的偏移位置
     *     // bOffset==0说明这段文字没有相应的Chpx
     *     bOffset (1 byte);
     *   }
     *   aChpx: Chpx[] {
     *     cb (1byte);
     *     grpprl: Prl[] {
     *       sprm (2 bytes);
     *       operand (sprm.spra .bytes);
     *     }
     *   }
     *   ..
     *   crun(last 1byte); // 0x01 <= crun <= 0x65
     * }
     */

    uchar *ChpxFkp = wds + PnBteChpx * 512;
    uchar crun = *(uchar *)(ChpxFkp + 511);
    fprintf(stdout, "crun = 0x%02X\n", (int)crun);

    for (size_t i = 0; i < crun; ++i) {
      // rgfc确定文本的位置
      uint rgfc_start = *(uint *)(ChpxFkp + 4 * i);
      uint rgfc_end = *(uint *)(ChpxFkp + 4 * i + 4);
      printf("[rgfc_start = 0x%X, rgfc_end = 0x%X]\n", rgfc_start, rgfc_end);
      for (uint cp = rgfc_start; cp != rgfc_end; cp += 2) {
        printf("    <0x%04X> = %s\n", cp, to_string(*(ushort *)(wds + cp)).c_str());
      }

      // bOffset找到ChpxInFkp在ChpxFkp中的位置
      uchar bOffset = *(uchar *)(ChpxFkp + 4 * (crun + 1) + 1 * i);
      if (bOffset == 0) {
        continue;
      }

      // 找到ChpxInFkp其中的cb, grpprlInChpx
      uchar cb = *(uchar *)(ChpxFkp + bOffset * 2);
      // printf("bOffset*2 = %d, cb=%d, cd'=%d\n", (int)bOffset * 2, cb, *(ChpxFkp + bOffset*2+1));

      // ..
      uchar *Chpx = ChpxFkp + 2 * bOffset;
      read_grpprl(Chpx + 1, cb);
    }
  }

  free(wds);
  free(table_stream);
  return true;
}
// }}}

// Determining Properties of a Style {{{
bool style_property(const Storage &st)  {
  uchar *wds = c_read_stream(&st, "WordDocument");
  if (wds == NULL) {
    return false;
  }

  // 取得Table Stream
  uint16_t flag1 = *(ushort *)((char *)wds + 0xA);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  uchar *table_stream = c_read_stream(&st, is1Table ? "1Table" : "0Table");
  if (table_stream == NULL) {
    return false;
  }

  uint fcStshfOrig = *(uint *)(wds + 154 + 0);
  uint lcbStshfOrig = *(uint *)(wds + 154 + 0 + 4);

  printf("fcStshfOrig = 0x%02X, lcbStshfOrig = 0x%02X\n", fcStshfOrig, lcbStshfOrig);

  uint fcStshf = *(uint *)(wds + 154 + 8);
  uint lcbStshf = *(uint *)(wds + 154 + 8 + 4);
  uchar *STSH = table_stream + fcStshf;

  printf("fcStshf = 0x%02X, lcbStshf = 0x%02X\n", fcStshf, lcbStshf);

  /*
   * STSH {
   *   LPStshi lpstshi (variable) {
   *     cbStshi (2 bytes); // stshi占用多少个byte
   *     STSHI stshi (variable) { 
   *       stshif (18 bytes) {
   *         cstd (2 bytes); // STSH.rglpstd有多少个元素，取值范围在[0xF,0xFFE]
   *         cbSTDBaseInFile (2 bytes); // Stdf占用多少byte
   *         A fStdStylenamesWritten (1 bit); // MUST be 1
   *         fReserved (15 bits);
   *         stiMaxWhenSaved (2 bytes);
   *         istdMaxFixedWhenSaved (2 bytes);
   *         nVerBuiltInNamesWhenSaved (2 bytes);
   *         ftcAsci (2 bytes);
   *         ftcFE (2 bytes);
   *         ftcOther (2 bytes);
   *       }
   *       ftcBi (2 bytes); // sprmCFtcBi的参数的取值
   *       StshiLsd (variable) { // 定义隐含的style
   *         cbLSD (2 bytes); // LSD结构的大小，一定是4
   *         LSD[] mpstiilsd { // LSD的数组
   *           // 元素个数等于Stshi.stiMaxWhenSaved
   *         }
   *       }
   *       StshiB (variable); // 没有用
   *     }
   *   }
   *   LPStd[] rglpstd (variable) {
   *     cbStd (2 bytes); // std占用多少字节
*  *     STD std (cbStd bytes) { // 定义Style的真正位置
   *       Stdf stdf (variable) {
   *         StdfBase stdfBase (10 bytes) {
   *           sti (12 bits); // style identifier
   *           A (1 bit); // ignore
   *           B (1 bit); // ignore
   *           C (1 bit); // ignore
   *           D (1 bit); // ignore
   *           stk (4 bits); // the type of this style
   *                        // 1: paragraph style
   *                        // 2: character style
   *                        // 3: table style
   *                        // 4: numbering style
   *           istdBase (12 bits); // this style's parent style's istd
   *           cupx (4 bits); // count of formatting sets inside the structure
   *           istdNext (12 bits);
   *           bchUpe (2 bytes); // LPStd中std的size，等于LPStd中的cbStd
   *           grfstd (2 bytes); // 其他style
   *         }
   *         StdfPost200OrNone (optional, 8 bytes);
   *       }
   *       Xstz xstzName (variable); // specify the primary style name by aliases
   *       GrLPUpxSw grLPUpxSw (variable);
   *     }
   *   }
   * }
   */

  // 跳过lpstshi，它的长度为cbStshi = *(ushort *)STSH;
  ushort cbStshi = *(ushort *)STSH;
  uchar *rglpstd = STSH + 2 + cbStshi;
  ushort cstd = *(ushort *)(STSH + 2);

  printf("cstd = %u\n", cstd);

  int counter = 0;
  while (rglpstd < STSH + lcbStshf) {
    ushort cbStd = *(ushort *)rglpstd;
    printf("cbStd = %3d  ", cbStd);

    uchar *std = rglpstd + 2;
    printf("stdfBase[%02X]: ", counter++);

    ushort sti = *(ushort *)std & 0xFFF;
    uchar stk = *(uchar *)(std + 2) & 0xF;
    ushort istdBase = *(ushort *)(std + 2) >> 4;
    uchar cupx = *(uchar *)(std + 4) & 0xF;

    printf("sti=%X,stk=%X,istdBase=%X,cupx=%X\n", sti,stk,istdBase,cupx);

    rglpstd += 2 + cbStd;
  }
  printf("counter = %d\n", counter);

  free(wds);
  free(table_stream);
  return true;
}
// }}}

// retrieve property info {{{
bool retrieve_property_info(const Storage &st, FILE *file) {
  if (file == NULL) {
    return false;
  }

  // 取得WordDocument Stream
  uchar *wds = c_read_stream(&st, "WordDocument");
  if (wds == NULL) {
    return false;
  }

  // 取得Table Stream
  uint16_t flag1 = *(ushort *)((char *)wds + 0xA);
  bool is1Table = (flag1 & 0x0200) == 0x0200;
  uchar *ts = c_read_stream(&st, is1Table ? "1Table" : "0Table");
  if (ts == NULL) {
    return false;
  }

  uint fcSttbfAssoc = *(uint *)(wds + 154 + 64 * 4);
  uint lcbSttbfAssoc = *(uint *)(wds + 154 + 65 * 4);

  /*
   * STTB {
   *   fExtend;
   *   cData;
   *   cbExtra;
   *   [0 <= i < cData] {
   *     cchData[i];
   *     Data[i];
   *     ExtraData[i];
   *   }
   * }
   *
   * STTB结构的一般情况：
   * fExtend的头两个字节等于0xFFFF时：
   *     Data字段中的字符是双字节的，cchData占两个字节
   * 如果STTB的头两个字节不等于0xFFFF：
   *     fExtend不存在，Data字段是单字节的，cchData占一个字节
   *
   * 在doc文件中需要满足：
   * fExtend (2 bytes) = 0xFFFF
   * cData (2 bytes) = 0x0012
   * cbExtra (2 bytes) = 0
   */

  uchar *SttbfAssoc = ts + fcSttbfAssoc;
  ushort fExtend = *(ushort *)SttbfAssoc;
  ushort cData = *(ushort *)(SttbfAssoc + 2);
  ushort cbExtra = *(ushort *)(SttbfAssoc + 4);

  if (fExtend != 0xFFFF) {
    fprintf(stderr, "WARNING: fExtend = 0x%04X, not 0xFFFF\n", fExtend);
  }
  if (cData != 0x0012) {
    fprintf(stderr, "WARNING: cData = 0x%04X, not 0x0012\n", cData);
  }
  if (cbExtra != 0x0000) {
    fprintf(stderr, "WARNING: cbExtra = 0x%04X, not 0x0000\n", cbExtra);
  }

  uchar *cur = SttbfAssoc + 6;
  for (uint i = 0; i < cData; i += 1) {
    ushort cchData = *(ushort *)(cur);
    cur += 2;
    if (i == 2 || i == 3 || i == 4 || i == 6 || i == 7) {
      printf("cchData = %d\n", cchData);
      for (uint j = 0; j < cchData; j += 1) {
        fputc(cur[j], file);
      }
    }
    cur += cchData;
    if (i == 0x02) { // Title
    } else if (i == 0x03) { // Subject
    } else if (i == 0x04) { // Key words
    } else if (i == 0x06) { // author
    } else if (i == 0x07) { // user who last revised the document
    }
  }

  free(wds);
  free(ts);
  return true;
}
// }}}

// parse \005SummaryInformation Stream {{{
bool parse_summary_information(const Storage &st, FILE *file) {
  if (file == NULL) {
    return false;
  }

  // 取得\005SummaryInformation Stream
  char sis_name[64];
  sis_name[0] = 5, sis_name[1] = 0;
  sprintf(sis_name + 2, "SummaryInformation");
  uchar *sis = c_read_stream(&st, sis_name);
  if (sis == NULL) {
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

  uchar *ps = sis + 48;
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
      uchar *value = ps + offset + 4;
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

  free(sis);
  return true;
}
// }}}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "at least 2 files: arg[1]=input file, arg[2]=output file\n");
    return 1;
  }

  Storage s;
  {
    // 打开输入文件
    FILE *file = fopen(argv[1], "rb");
    if (!s.init(file)) {
      fclose(file);
      fprintf(stderr, "init fail!\n");
      return -1;
    }
    fclose(file);
  }

  // 输入文件中有哪些流
  // s.__print_dir();

  // 输入文件中中各个流的之间的层次结构
  list_directory_tree(s, 0, 0);

  /*
  uchar *wds = c_read_stream(&s, "WordDocument");
  for (uint i = 0; i < 190; ++i) {
    printf("FibRgLcb97[%03d] = %08X\n", i, *(uint *)(wds + 154 + i * 4));
  }
  free(wds);
  */

  // 取得文本，以UTF-16LE保存
  std::vector<uint8_t> str;
  if (!retrieve_text(s, str)) {
    fprintf(stderr, "can't retrieve text!\n");
  } else {
    // 输出文件
    FILE *tmp_file = fopen(argv[2], "wb");

    // 输出property information
    if (!retrieve_property_info(s, tmp_file)) {
      fprintf(stderr, "Can't retrieve propery information\n");
    }

    if (!parse_summary_information(s, tmp_file)) {
      fprintf(stderr, "Can't parse summary information\n");
    }

    // 输出文本
    for (uint i = 0; i < str.size(); ++i) {
      fputc(str[i], tmp_file);
    }
    if (!WriteStream(tmp_file, str)) {
      return -1;
    }
    fclose(tmp_file);
  }

  // 取得图片
  if (!retrieve_image(s, ".")) {
    fprintf(stderr, "can't retrieve images!\n");
  }

  // Formatting： 文本的位置
  // character_formatting(s);
  // paragraph_formatting(s);
  // parse_format(s, tmp_file);

  /*
  printf("sprms:\n");
  for (auto p: sprms) {
    printf("0x%04X\n", p);
  }
  */

  // Style： 文本的形状
  // style_property(s);

  return 0;
}
