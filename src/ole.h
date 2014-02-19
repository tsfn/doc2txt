#ifndef OLE_H
#define OLE_H

#include <cstdio>
#include <cassert>
#include <cstring>

#include <set>
#include <vector>
#include <string>
#include <algorithm>


#define NEED_WARNING
#define __in
#define __out

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// 类型定义
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef short OFFSET;
typedef ULONG SECT;
typedef ULONG FSINDEX;
typedef USHORT FSOFFSET;
typedef ULONG DFSIGNATURE;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef WORD DFPROPTYPE;
typedef ULONG SID;
//typedef CLSID GUID;

// SAT中的表项的取值
// >= 0指向下一个sector
const SECT DIFSECT = 0xFFFFFFFC; // 用来存MSAT用到的sector
const SECT FATSECT = 0xFFFFFFFD; // 用来存SAT用到的sector
const SECT ENDOFCHAIN = 0xFFFFFFFE; // 表示这个sector是这个链的最后一个sector了
const SECT FREESECT = 0xFFFFFFFF; // 表示这个sector没有存东西

// DirectoryEntry的类型
typedef enum tagSTGTY {
  STGTY_INVALID	 = 0,
  STGTY_STORAGE	 = 1,
  STGTY_STREAM	 = 2,
  STGTY_LOCKBYTES	 = 3,
  STGTY_PROPERTY	 = 4,
  STGTY_ROOT		 = 5,
} STGTY;

// DirEntry的颜色（红黑树中用到）
typedef enum tagDECOLOR {
  DE_RED = 0,
  DE_BLACK = 1,
} DECOLOR;

// DirEntry的修改时间
typedef struct tagFILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, TIME_T;


/*
 * DirEntry类
 */

class DirEntry {
public:
  uint8_t _ab[64];
  //[0x00]这个directory_entry所表示的流的名字，按UTF-16保存
  uint16_t _cb; //[0x40] _ab保存的名字的长度，按有多少字算，不是按字节算
  uint8_t _mse; //[0x42] 对象的类型，值取STGTY中的一个
  uint8_t _bflags; //[0x43] 值是DECOLOR中的一个
  uint32_t _sidLeftSib; //[0x44] 当前项的左兄弟的SID
  uint32_t _sidRightSib; //[0x48] 当前项的右兄弟的SID
  uint32_t _sidChild; //[0x4C] 当前层中的根节点
  uint8_t _clsId[16]; //[0x50] 当前storage的CLSID（如果_mse=STGTY_STORAGE）
  uint32_t _dwUserFlags; //[0x60] 当前storage的用户标记（如果_mse=STGTY_STORAGE）
  TIME_T _time[2]; //[0x64] Create/Modify时间戳
  uint32_t _sectStart; //*[0x74] 流的起始sector的SecID
  uint32_t _ulSize; //*[0x78] 流的大小，单位是字节
  uint16_t _dptPropType; //[0x7C] 一定都是0

public:
  bool init(const std::vector<uint8_t> &s);
};


/*
 * Storage类
 */

class Storage {
private:
  std::vector<uint8_t> _buf; //doc file
  std::vector<uint8_t> _miniBuf; //short-stream container stream
  std::vector<uint8_t> _sat; //sector allocation table
  std::vector<DirEntry> _dir; //all of the directories
  std::vector<uint8_t> _ssat; //short-sector allocation table

private:
  unsigned long _nsec; // number of sectors in _buf
  unsigned long _sz; // size of a sector
  unsigned long _ssz; // size of a short-sector

private:
  //static const uint8_t _abSig[8] = {0xD0, 0xCF, 0x11, 0xE0,
  //                                  0xA1, 0xB1, 0x1A, 0xE1};
  //CLSID _clid;
  //USHORT _uMinorVersion;
  //USHORT _uDllVersion;
  //USHORT _uByteOrder; // 一般是0xFFFE，表示小端存储；0xFEFF表示大端存储
  //USHORT _uSectorShift; // -> _sz
  //USHORT _uMiniSectorShift; // -> _ssz
  //USHORT _usReserved;
  //ULONG _ulReserved1;
  //ULONG _ulReserved2;
  //FSINDEX _csectFat; // SAT用掉了几个sector
  //SECT _sectDirStart; // -> MSAT chain's first SecID
  //DFSIGNATURE _signature; // -> MSAT chain's length
  ULONG _ulMiniSectorCutoff; // short-sector的最大大小
  //ULONG _sectMiniFatStart; // *[0x3C] SSAT占用的第一个sector的SecID
  //ULONG _csectMiniFat; // *[0x40] SSAT占用了多少个sector
  //ULONG _sectDifStart; // *[0x44] MSAT占用的第一个sector的SecID
  //ULONG _csectDif; // *[0x48] MSAT占用了多少个sector
  //ULONG _sectFat[109]; // *[0x4C] MSAT的第一个部分，包含109个SecID

public: // debug
  void __print_dir() {
    for (int i = 0; i < (int)_dir.size(); ++i) {
      char temp[32];
      for (int j = 0; j < 32; ++j) {
        temp[j] = _dir[i]._ab[j * 2];
      }
      printf("[%d] %s: ls=[%d], rs=[%d], ch=[%d]\n",
          i, temp,
          _dir[i]._sidLeftSib, _dir[i]._sidRightSib, _dir[i]._sidChild);
    }
    printf("\n");
  }

private:
  bool _init();
  bool _init_sat();
  bool _init_dir();
  bool _init_ssat();
  bool __read_stream(
      int sec_id,
      int sec_sz,
      const std::vector<uint8_t> &sat,
      const std::vector<uint8_t> &buf,
      int _offset,
      __out std::vector<uint8_t> &stream) const;

public:
  bool init(FILE *file);
  bool init(const std::vector<uint8_t> &arg_buf);
  bool init(uint8_t *arg_buf, uint32_t length);

  bool read_stream(const char *name, __out std::vector<uint8_t> &stream) const;
  bool read_stream(unsigned int DirID, __out std::vector<uint8_t> &stream) const;

  bool get_dir_entry(unsigned int DirID, __out DirEntry &entry) const;
  bool entry_children(unsigned int DirID, __out std::vector<uint32_t> &children) const;
};

namespace tsfn {
  uint16_t get16(const std::vector<uint8_t> &buf, unsigned int offset);
  uint32_t get32(const std::vector<uint8_t> &buf, unsigned int offset);
}
#endif
