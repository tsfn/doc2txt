#include "ole.hpp"



/*
 * 从字节数组中提取整数的小工具
 */

uint16_t tsfn::get16(const std::vector<uint8_t> &buf, unsigned int offset) {
  uint16_t a[2] = {};
  for (int i = 0; i < 2; ++i) {
    a[i] = offset < buf.size() ? buf[offset] : 0;
    ++offset;
  }
  return (a[1] << 8) | a[0];
}

uint32_t tsfn::get32(const std::vector<uint8_t> &buf, unsigned int offset) {
  uint32_t a[4] = {};
  for (int i = 0; i < 4; ++i) {
    a[i] = offset < buf.size() ? buf[offset] : 0;
    ++offset;
  }
  return (a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
}


using tsfn::get16;
using tsfn::get32;


/*
 * class Storage;
 * OLE文件（doc/ppt/xls）结构解析器的实现
 * 初始化函数：
 *    bool init(FILE *file);
 *    bool init(const std::vector<uint8_t> &buf);
 *    bool init(uint8_t *buf, uint32_t length);
 */

bool Storage::init(FILE *file) {
  if (file == NULL) {
    return false;
  }
  fseek(file, 0, SEEK_SET);
  int ch;
  while ((ch = fgetc(file)) != EOF) {
    _buf.push_back((uint8_t)ch);
  }
  return _init();
}

bool Storage::init(const std::vector<uint8_t> &buf) {
  _buf = buf;
  return _init();
}

bool Storage::init(uint8_t *buf, uint32_t l) {
  if (buf == NULL) {
    return buf;
  }
  _buf.resize(l);
  for (int i = 0; i < (int)l; ++i) {
    _buf[i] = buf[i];
  }
  return _init();
}


/*
 * 根据buf中的内容初始化
 */
bool Storage::_init() {
  // 从实际数据发现OLE文件的大小不一定是整512字节的，从_buf中取字节的时候要注意
  if (_buf.size() < 512U) {
    return false;
  }

  // 1:提取字段值，检测值的合法性

  // UTF-16中BOM=0xFEFF
  // 微软说在这里OxFFFE代表小端存储
  // 但事实上大家都只用小端方式
  uint16_t _uByteOrder = ((uint16_t)_buf[0x1D] << 8) + _buf[0x1C];
  if (_uByteOrder != 0xFFFE) {
    return false;
  }

  // 一个普通sector占用多少字节
  uint16_t _uSectorShift = get16(_buf, 0x01E);
  if (_uSectorShift == 0 || _uSectorShift >= 32) {
    return false;
  }
  _sz = 1UL << _uSectorShift;

  // 一个short-sector占用多少字节
  uint16_t _uMiniSectorShift = get16(_buf, 0x020);
  if (_uMiniSectorShift == 0 || _uMiniSectorShift >= 32) {
    return false;
  }
  _ssz = 1UL << _uMiniSectorShift;

  // short-sector的最大长度
  _ulMiniSectorCutoff = get32(_buf, 0x038);
  if (_ulMiniSectorCutoff == 0) {
    return false;
  }

  // 2:将_buf的大小扩展成(512 + 若干个_sz)字节的
  _nsec = ((_buf.size() - 512) + _sz - 1) / _sz;
  _buf.resize(512 + _nsec * _sz);


  // 3:检查头部的标识符
  static const uint8_t _abSig1[8] =
  {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
  static const uint8_t _abSig2[8] = 
  {0x0E, 0x11, 0xFC, 0x0D, 0xD0, 0xCF, 0x11, 0xE0};
  bool sigFlag1 = true, sigFlag2 = true;
  for (int i = 0; i < 8; ++i) {
    sigFlag1 &= (_buf[i] == _abSig1[i]);
    sigFlag2 &= (_buf[i] == _abSig2[i]);
  }
  if (!sigFlag1 && !sigFlag2) {
    return false;
  }

  // 初始化：SAT（sector allocation table）、
  // 目录项directory entry、
  // SSAT（short-sector allocation table）
  return _init_sat() && _init_dir() && _init_ssat();
}


/*
 * 初始化sector分配表(sector allocation table，SAT)
 */
bool Storage::_init_sat() {
  // 一：得到MSAT
  std::vector<uint8_t> _msat;
  {
    // MSAT分成header中的部分和链表部分
    // 只有文件比较大的时候才会用到链表部分

    //header中MSAT部分的大小
    const int osz = 109 * 4;

    // 得到MSAT的header中的部分
    // 保证_buf的大小合法
    for (int j = 0; j < osz; ++j) {
      _msat.push_back(_buf[0x4C + j]);
    }

    // MSAT链表部分用到的第一个SecID
    uint32_t _sectDifStart = get32(_buf, 0x044);
    if (_sectDifStart != ENDOFCHAIN && _sectDifStart >= _nsec) {
      return false;
    }

    // 得到sector中的MSAT的链表部分
    // 同时检测从_sectDifStart开始的MSAT链的合法性
    uint32_t cnt_sect = 0;
    std::set<int> vis;
    for (uint32_t i = _sectDifStart;; i = get32(_buf, 512 + _sz * i + _sz - 4)) {
      if ((int)i >= 0) {
        if (i >= _nsec || vis.count(i)) {
          return false;
        }
        vis.insert(i);
      } else if (i == ENDOFCHAIN) {
        break;
      } else {
        return false;
      }
      cnt_sect += 1;
      for (uint32_t j = 0; j < _sz - 4; ++j) {
        _msat.push_back(_buf[512 + _sz * i + j]);
      }
    }

    // MSAT用到的sector的数量如果和header._csectDif不符
    // 只输出一条提示信息
    uint32_t _csectDif = get32(_buf, 0x048); // MSAT用到了几个sector
    if (cnt_sect != _csectDif) {
#ifdef NEED_WARNING
      fprintf(stderr,
          "Warning: header._csectDif is %u != "
          "real number of sectors used by MSAT(%u)\n",
          _csectDif, cnt_sect);
      fflush(stderr);
#else
      return false;
#endif
    }
  }

  // 二：得到SAT
  {
    // SAT占用了多少sector
    uint32_t _csectFat = get32(_buf, 0x2C);
    if (_csectFat > _msat.size() / 4) {
      return false;
    }

    // 通过MSAT将SAT取出来
    _sat.clear();
    for (uint32_t i = 0; i < _csectFat; ++i) {
      uint32_t id = get32(_msat, i * 4);
      if (id >= _nsec) {
        return false;
      }
      for (uint32_t j = 0; j < _sz; ++j) {
        _sat.push_back(_buf[512 + id * _sz + j]);
      }
    }
  }

  // 三：对MSAT中的内容进行检查
  {
    // SAT占用了多少sector
    uint32_t _csectFat = get32(_buf, 0x2C);

    for (uint32_t i = 0; i < _msat.size() / 4; ++i) {
      // MSAT说SecID=id的sector是用来存放SAT的
      uint32_t id = get32(_msat, i * 4);

      if (i < _csectFat) {
        // SAT中SecID=id的sector的表记了的值，应该是FATSECT
        uint32_t val = get32(_sat, id * 4);
        if (val != FATSECT) {
#ifdef NEED_WARNING
          fprintf(stderr,
              "Warning: SecId=%d, used for SAT, but SAT[%d]=%d\n",
              id, id, val);
          fflush(stderr);
#else
          return false;
#endif
        }
      } else {
        // MSAT在这里不指向任何值，用FREESECT填充
        if (id != FREESECT) {
#ifdef NEED_WARNING
          fprintf(stderr, "Warning: _csectFat = %u, _msat[%d] = %d\n",
              _csectFat, i, id);
          fflush(stderr);
#else
          return false;
#endif
        }
      }
    }
  }

  // 四：对SAT中的MSAT占用的数据进行合法性检测
  {
    // MSAT链表部分用到的第一个SecID
    uint32_t _sectDifStart = get32(_buf, 0x044);

    // 遍历MSAT用到的sector，然后对比SAT中相应表项的值是否合法
    for (uint32_t i = _sectDifStart; i != ENDOFCHAIN;
        i = get32(_buf, 512 + _sz * i + _sz - 4)) {
      // SAT的第i项应该是-4，因为放了MSAT用到的sector
      uint32_t val = get32(_sat, i * 4);
      if (val != DIFSECT) {
#ifdef NEED_WARNING
        fprintf(stderr,
            "Warning: SecId=%d, "
            "used for MSAT, but SAT[%d]=%d\n",
            i, i, val);
        fflush(stderr);
#else
        return false;
#endif
      }
    }
  }

  // 五：检查SAT中的链是否都合法
  {
    // 将_sat转化成数组，检查每项的合法性
    std::vector<uint32_t> sat;
    uint32_t node_counter = 0; // 可能是链上的节点的数量
    for (unsigned long i = 0; i < _sat.size(); i += 4) {
      int id = get32(_sat, i);
      if (id < -4 || id >= (int)_sat.size() / 4) {
        return false;
      }
      if (id == (int)ENDOFCHAIN || id >= 0) {
        node_counter += 1;
      }
      sat.push_back(id);
    }

    // 记录哪些项没有入度
    std::vector<uint8_t> ind(sat.size(), 0);
    for (int i = 0; i < (int)sat.size(); ++i) {
      int id = sat[i];
      if (id >= 0) {
        ind[id] = 1;
      }
    }

    // 遍历每个SAT上的链，出现环的话返回false
    std::set<uint32_t> vis; // 记录遍历过的位置
    for (int i = 0; i < (int)sat.size(); ++i) {
      if (((int)sat[i] >= 0 || sat[i] == ENDOFCHAIN) && ind[i] == 0) {
        // 从这个SecID开始应该可以连成一条链，不能有环
        for (uint32_t id = i; id != ENDOFCHAIN; id = sat[id]) {
          if (vis.count(id)) { // 出现环
            return false;
          }
          vis.insert(id);
        }
      }
    }

    // 如果遍历过的位置的数量不等于所有可能是链节点的节点数量
    if (vis.size() != node_counter) {
#ifdef NEED_WARNING
      for (uint32_t i = 0; i < sat.size(); ++i) {
        printf("sat[%d] = %d\n", i, sat[i]);
      }
      fprintf(stderr, "Warning: SAT may be invalid:"
          "vis.size() = %u, node_counter = %d\n",
          vis.size(), node_counter);
      fflush(stderr);
#else
      return false;
#endif
    }
  }

  return true;
}

/*
 * 初始化所有目录项
 */

bool Storage::_init_dir() {
  uint32_t _sectDirStart = get32(_buf, 0x030);
  std::vector<uint8_t> str;
  if (!__read_stream(_sectDirStart, _sz, _sat, _buf, 512, str)) {
    return false;
  }
  _dir.clear();
  DirEntry dir_entry;
  for (int i = 0; i < (int)str.size(); i += 128) {
    std::vector<uint8_t> ts;
    for (int j = 0; j < 128; ++j) {
      ts.push_back(i + j >= (int)str.size() ? 0 : str[i + j]);
    }
    if (!dir_entry.init(ts)) {
      return false;
    }
    _dir.push_back(dir_entry);
  }
  return true;
}

bool Storage::_init_ssat() {
  // 初始化短流要用到的 SSAT(short-sector allocation table)
  //    和SSCS(short-stream container stream)
  //  过程和读取普通的流一样
  uint32_t _sectMiniFatStart = get32(_buf, 0x03C);
  uint32_t _csectMiniFat = get32(_buf, 0x040);
  if (!__read_stream(_sectMiniFatStart, _sz, _sat, _buf, 512, _ssat)) {
    return false;
  }
  if (!__read_stream(_dir[0]._sectStart, _sz, _sat, _buf, 512, _miniBuf)) {
    return false;
  }

  if ((_ssat.size() + _sz - 1) / _sz != _csectMiniFat) {
#ifdef NEED_WARNING
    //SSAT实际没有用到_csectMiniFat这么多的sector
    fprintf(stderr, "Warning: _csectMiniFat = %u,"
        "_ssat.size() = %u\n",
        _csectMiniFat,
        _ssat.size());
#else
    return false;
#endif
  }
  return true;
}

bool Storage::__read_stream(
    int id,
    int sz,
    const std::vector<uint8_t> &sat,
    const std::vector<uint8_t> &buf,
    int _offset,
    __out std::vector<uint8_t> &stream) const {
  stream.clear();
  while (id != (int)ENDOFCHAIN) {
    // SecID的范围要合法
    if (id < 0 || id >= (int)sat.size() / 4) {
      return false;
    }
    for (int j = 0; j < sz; ++j) {
      // SecID指向的范围要在buf的范围中
      if (_offset + id * sz + j < (int)buf.size()) {
        stream.push_back(buf[_offset + id * sz + j]);
      } else {
        return false;
      }
    }
    id = get32(sat, 4 * id);
  }
  return true;
}

/*
 * 根据流的名字读取流
 * bool Storage::read_stream(
 const char *name,
 __out std::vector<uint8_t> &stream);
 */
bool Storage::read_stream(
    const char *name,
    __out std::vector<uint8_t> &stream) const {
  // 根据名字提取流的过程其实就是根据名字在_dir中寻找DirID
  // 然后调用bool Storage::read_stream(DirID, stream)
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
    return read_stream(i, stream);
  }
  return false;
}


/*
 * 读取当前Storage的目录项中编号为DirID的目录项
 * 如果要通过目录结构分析文件的话可能要用到这个函数
 */
bool Storage::read_stream(
    unsigned int DirID,
    __out std::vector<uint8_t> &stream) const {
  if (DirID >= _dir.size()) {
    return false;
  }
  int SecID = _dir[DirID]._sectStart;
  return _dir[DirID]._ulSize > _ulMiniSectorCutoff ?
    __read_stream(SecID, _sz, _sat, _buf, 512, stream) :
    __read_stream(SecID, _ssz, _ssat, _miniBuf, 0, stream);
}


/*
 * 根据DirID取得某个目录项（DirEntry）
 bool Storage::get_dir_entry(int DirID, __out DirEntry &entry);
 */
bool Storage::get_dir_entry(unsigned int DirID, __out DirEntry &entry) const {
  if (DirID >= _dir.size()) {
    return false;
  }
  entry = _dir[DirID];
  return true;
}

/*
 * Storage的第DirID个目录项的所有子目录项的DirID
 */
bool Storage::entry_children(
    unsigned int DirID,
    __out std::vector<uint32_t> &children) const {
  if (DirID >= _dir.size()) {
    return false;
  }

  // 在文件系统中，一个目录下的所有项目是用链表连在一起的
  // 为了加快查询速度，MS把一个目录下的所有项目存在了一个平衡二叉树里（红黑树）
  // 对这一层的所有项目组成的二叉树进行一次宽搜，得到所有子项目
  std::set<uint32_t> visit;
  children.clear();
  if (_dir[DirID]._sidChild < _dir.size()) {
    children.push_back(_dir[DirID]._sidChild);
  }
  visit.insert(_dir[DirID]._sidChild);
  for (int i = 0; i < (int)children.size(); ++i) {
    int child[2], cur = children[i];
    child[0] = _dir[cur]._sidLeftSib;
    child[1] = _dir[cur]._sidRightSib;
    for (int j = 0; j < 2; ++j) {
      uint32_t id = child[j];
      if (id < _dir.size()) {
        // 每个点的后继各不相同
        if (visit.count(id)) {
          return false;
        }
        visit.insert(id);
        children.push_back(id);
      } else if ((int)id == -1) { // DirID标志结尾，跳过不处理
      } else {
#ifdef NEED_WARNING
        fprintf(stderr, "Warning: DirID[%u]'s %uth children's id = %08XU\n",
            DirID, j, id);
        fflush(stderr);
#else
        return false;
#endif
      }
    }
  }
  return true;
}


/*
 * class DirEntry;
 * 目录项的构造函数
 */

bool DirEntry::init(const std::vector<uint8_t> &s) {
  if (s.size() != 128U) {
    return false;
  }
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
  return true;
}

uchar *c_read_stream(const Storage *st, const char *name) {
  std::vector<uint8_t> wds;
  if (!st->read_stream(name, wds)) {
    return NULL;
  }
  uchar *s = (uchar *)malloc(wds.size());
  for (size_t i = 0; i < wds.size(); ++i) {
    s[i] = wds[i];
  }
  return s;
}

