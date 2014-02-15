#include "ole.h"

using namespace tsfn;

/*
 * 从字节数组中提取整数的小工具
 */
uint16_t tsfn::get16(const std::vector<uint8_t> &buf, unsigned int offset) {
  assert(offset + 2U <= buf.size());
  uint16_t a[2] = {};
  for (int i = 0; i < 2; ++i) a[i] = buf[offset++];
  return (a[1] << 8) | a[0];
}

uint32_t tsfn::get32(const std::vector<uint8_t> &buf, unsigned int offset) {
  assert(offset + 4U <= buf.size());
  uint32_t a[4] = {};
  for (int i = 0; i < 4; ++i) a[i] = buf[offset++];
  return (a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
}


/*
 * class Storage;
 * OLE文件（doc/ppt/xls）结构解析器的实现
 * 构造函数：
 *    Storage(FILE *file);
 *    Storage(const std::vector<uint8_t> &buf);
 *    Storage(uint8_t *buf, uint32_t length);
 */

Storage::Storage(FILE *file) {
  assert(file != NULL);
  fseek(file, 0, SEEK_SET);
  int ch;
  while ((ch = fgetc(file)) != EOF) {
    _buf.push_back((uint8_t)ch);
  }
  _init();
}

Storage::Storage(const std::vector<uint8_t> &buf) {
  _buf = buf;
  _init();
}

Storage::Storage(uint8_t *buf, uint32_t l) {
  assert(buf != NULL);
  _buf.resize(l);
  for (int i = 0; i < (int)l; ++i) {
    _buf[i] = buf[i];
  }
  _init();
}


void Storage::_init() { // 根据buf中的内容初始化
  // 从实际数据发现OLE文件的大小不一定是整512字节的，从_buf中取字节的时候要注意
  assert(_buf.size() >= 512U);
  static const uint8_t _abSig[8] = {0xD0, 0xCF, 0x11, 0xE0,
                                    0xA1, 0xB1, 0x1A, 0xE1};
  for (int i = 0; i < 8; ++i) {
    assert(_buf[i] == _abSig[i]);
  }
  uint16_t _uSectorShift = get16(_buf, 0x01E);
  uint16_t _uMiniSectorShift = get16(_buf, 0x020);
  uint32_t _csectFat = get32(_buf, 0x02C);
  _ulMiniSectorCutoff = get32(_buf, 0x038);
  _sz = 1UL << _uSectorShift; // 一个普通sector占用多少字节
  _ssz = 1UL << _uMiniSectorShift; // 一个short-sector占用多少字节
  _initSAT();
  _initDIR();
  _initSSAT();
}

void Storage::_initSAT() { // 初始化sector分配表(sector allocation table)
  //先搞出MSAT，MSAT分成header中的部分和链表部分，
  //只有文件比较大的时候才会用到链表部分
  uint32_t _sectDifStart = get32(_buf, 0x044); // first SecID of MSAT
  uint32_t _csectDif = get32(_buf, 0x048); // number of sectors used by MSAT
  const int osz = 109 * 4;
  std::vector<uint8_t> _msat;
  for (int j = 0; j < osz; ++j) { // get the MSAT's SecID that is in header
    _msat.push_back(_buf[0x4C + j]);
  }
  uint32_t cnt_sect = 0;
  for (int i = _sectDifStart; i >= 0; i = get32(_buf, 512 + _sz*i + _sz-4)) {
    cnt_sect += 1;
    for (int j = 0; j < _sz - 4; ++j) {
      _msat.push_back(_buf[512 + i * _sz + j]);
    }
  }

#ifdef NEED_WARNING
  if (cnt_sect != _csectDif) {
    fprintf(stderr, "Warning: header._csectDif is %u != "
        "real number of sectors used by MSAT(%u)\n",
        _csectDif, cnt_sect);
    fflush(stderr);
  }
#endif

  //通过MSAT将SAT取出来
  FSINDEX _csectFat = get32(_buf, 0x2C);
  _sat.clear();
  for (int i = 0; i < (int)_csectFat; ++i) {
    int id = get32(_msat, i * 4);
    for (int j = 0; j < _sz && 512 + id * _sz + j < (int)_buf.size(); ++j) {
      _sat.push_back(_buf[512 + id * _sz + j]);
    }
  }

#ifdef NEED_WARNING
  for (int i = 0; i < (int)_msat.size() / 4; ++i) {
    int id = get32(_msat, i * 4); // MSAT说SecID=id的sector是用来存放SAT的
    if (i < (int)_csectFat) {
      // SAT中SecID=id的sector的表记了的值，应该是-3
      int val = get32(_sat, id * 4);
      if (val != -3) {
        fprintf(stderr, "Warning: SecId=%d"
            "used for SAT, but SAT[%d]=%d\n", id, id, val);
        fflush(stderr);
      }
    } else {
      if (id != -1) {
        fprintf(stderr, "Warning: _csectFat = %u,"
            "_msat[%d] = %d\n", _csectFat, i, id);
        fflush(stderr);
      }
    }
  }
#endif
}

void Storage::_initDIR() {
  uint32_t _sectDirStart = get32(_buf, 0x030);
  std::vector<uint8_t> str = __read_stream(_sectDirStart, _sz, _sat, _buf);
  _dir.clear();
  for (int i = 0; i < (int)str.size(); i += 128) {
    std::vector<uint8_t> ts;
    for (int j = 0; j < 128; ++j) {
      ts.push_back(i + j >= (int)str.size() ? 0 : str[i + j]);
    }
    DirEntry d = DirEntry(ts);
    if (d._mse != 0) _dir.push_back(d);
  }
}

void Storage::_initSSAT() {
  // 初始化短流要用到的 SSAT(short-sector allocation table)
  //    和SSCS(short-stream container stream)
  //  过程和读取普通的流一样
  uint32_t _sectMiniFatStart = get32(_buf, 0x03C);
  uint32_t _csectMiniFat = get32(_buf, 0x040);
  _ssat = __read_stream(_sectMiniFatStart, _sz, _sat, _buf);
  _miniBuf = __read_stream(_dir[0]._sectStart, _sz, _sat, _buf);

#ifdef NEED_WARNING
  if ((_ssat.size() + _sz - 1) / _sz != _csectMiniFat) {
    //SSAT实际没有用到_csectMiniFat这么多的sector
    fprintf(stderr, "Warning: _csectMiniFat = %u,"
        "_ssat.size() = %u\n", _ssat.size());
  }
#endif
}

std::vector<uint8_t> Storage::__read_stream(int i,
    int sz,
    const std::vector<uint8_t> &sat,
    const std::vector<uint8_t> &buf,
    int _offset) {
  std::vector<uint8_t> str;
  for (; i >= 0; i = get32(sat, 4 * i)) {
    for (int j = 0; j < sz && _offset + i * sz + j < (int)buf.size(); ++j) {
      str.push_back(buf[_offset + i * sz + j]);
    }
  }
  return str;
}

/*
 * 根据流的名字读取流
 * bool Storage::read_stream(
         const char *name,
         __out std::vector<uint8_t> &stream);
 */
bool Storage::read_stream(
    const char *name,
    __out std::vector<uint8_t> &stream) {
  for (int i = 0; i < (int)_dir.size(); ++i) {
    bool is_same_name = true;
    for (int j = 0; name[j] != '\0'; ++j) {
      if (_dir[i]._ab[2 * j] != name[j]) {
        is_same_name = false;
        break;
      }
    }
    if (!is_same_name) {
      continue;
    }

    int SecID = _dir[i]._sectStart;
    stream = _dir[i]._ulSize > _ulMiniSectorCutoff ?
      __read_stream(SecID, _sz, _sat, _buf) :
      __read_stream(SecID, _ssz, _ssat, _miniBuf, 0);
    return true;
  }
  return false;
}


/*
 * 读取当前Storage的目录项中编号为DirID的目录项
 * 如果要通过目录结构分析文件的话可能要用到这个函数
 */
bool Storage::read_stream(
    unsigned int DirID,
    __out std::vector<uint8_t> &stream) {
  if (DirID >= _dir.size()) {
    return false;
  }

  int SecID = _dir[DirID]._sectStart;
  stream = _dir[DirID]._ulSize > _ulMiniSectorCutoff ?
    __read_stream(SecID, _sz, _sat, _buf) :
    __read_stream(SecID, _ssz, _ssat, _miniBuf, 0);
  return true;
}


/*
 * 根据DirID取得某个目录项（DirEntry）
   bool Storage::get_dir_entry(int DirID, __out DirEntry &entry);
 */
bool Storage::get_dir_entry(unsigned int DirID, __out DirEntry &entry) {
  if (DirID >= _dir.size()) {
    return false;
  }
  entry = _dir[DirID];
  return true;
}

/*
 * Storage的第DirID个目录项的所有子目录项的DirID
 */
std::vector<uint32_t> Storage::entry_children(unsigned int DirID) {
  // 在文件系统中，一个目录下的所有项目是用链表连在一起的
  // 为了加快查询速度，MS把一个目录下的所有项目存在了一个平衡二叉树里（红黑树）
  std::vector<uint32_t> children;
  if (DirID < _dir.size() && _dir[DirID]._sidChild >= 0) {
    //对这一层的所有项目组成的二叉树进行一次宽搜，得到所有子项目
    children.push_back(_dir[DirID]._sidChild);
    for (int i = 0; i < (int)children.size(); ++i) {
      if (_dir[i]._sidLeftSib >= 0) {
        children.push_back(_dir[i]._sidLeftSib);
      }
      if (_dir[i]._sidRightSib >= 0) {
        children.push_back(_dir[i]._sidRightSib);
      }
    }
  }
  return children;
}


/*
 * class DirEntry;
 * 目录项的构造函数
 */

DirEntry::DirEntry(const std::vector<uint8_t> &s) {
  assert(s.size() == 128U);
  for (int i = 0; i < 64; ++i) {
    _ab[i] = s[i];
  }
  _cb = get16(s, 0x40);
  _mse = s[0x42];
  _bflags = s[0x43];
  _sidLeftSib = get32(s, 0x44);
  _sidRightSib = get32(s, 0x48);
  _sidChild = get32(s, 0x4C);
  for (int i = 0; i < 16; ++i) {
    _clsId[i] = s[0x50 + i];
  }
  _dwUserFlags = get32(s, 0x60);
  //_time[2];
  _sectStart = get32(s, 0x74);
  _ulSize = get32(s, 0x78);
  _dptPropType = get16(s, 0x7C);

#ifdef NEED_WARNING
  if (_dptPropType != 0) {
    fprintf(stderr, "Warning: DirEntry._dptPropType == %u\n", _dptPropType);
  }
#endif
}


