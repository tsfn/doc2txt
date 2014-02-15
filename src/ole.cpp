#include "ole.h"

namespace tsfn {
  uint16_t get16(const std::vector<uint8_t> &buf, int offset) {
    uint16_t a[2] = {};
    for (int i = 0; i < 2; ++i) a[i] = buf[offset++];
    return (a[1] << 8) | a[0];
  }
  uint32_t get32(const std::vector<uint8_t> &buf, int offset) {
    uint32_t a[4] = {};
    for (int i = 0; i < 4; ++i) a[i] = buf[offset++];
    return (a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
  }
}

using namespace tsfn;

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

void Storage::_init() {
  assert(_buf.size() >= 512U);
  static const uint8_t _abSig[8] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
  for (int i = 0; i < 8; ++i) {
    assert(_buf[i] == _abSig[i]);
  }
  _uSectorShift = get16(_buf, 0x01E);
  _uMiniSectorShift = get16(_buf, 0x020);
  _csectFat = get32(_buf, 0x02C);
  _ulMiniSectorCutoff = get32(_buf, 0x038);
  _sz = 1 << _uSectorShift;
  _ssz = 1 << _uMiniSectorShift;
  _initSAT();
  _initDIR();
  _initSSAT();
}
void Storage::_initSAT() {
  SECT _sectDifStart = get32(_buf, 0x044);
  FSINDEX _csectDif = get32(_buf, 0x048);
  const int osz = 109 * 4;
  std::vector<uint8_t> _msat;
  for (int j = 0; j < osz; ++j) _msat.push_back(_buf[0x4C + j]);
  uint32_t cnt_sect = 0;
  for (int i = _sectDifStart; i >= 0; i = get32(_buf, 512 + _sz * i + (_sz - 4))) {
    cnt_sect += 1;
    for (int j = 0; j < _sz - 4; ++j) {
      _msat.push_back(_buf[512 + i * _sz + j]);
    }
  }

#ifdef NEED_WARNING
  if (cnt_sect != _csectDif) {
    fprintf(stderr, "Warning: header._csectDif(%u) != real number of sectors used by MSAT(%u)\n", _csectDif, cnt_sect);
    fflush(stderr);
  }
#endif

  _sat.clear();
  for (int i = 0; i < (int)_csectFat; ++i) {
    int id = get32(_msat, i * 4);
    //assert(0 <= id && id < (int)_csectFat);
    for (int j = 0; j < _sz && 512 + id * _sz + j < (int)_buf.size(); ++j) {
      _sat.push_back(_buf[512 + id * _sz + j]);
    }
  }

#ifdef NEED_WARNING
  for (int i = 0; i < (int)_msat.size() / 4; ++i) {
    int id = get32(_msat, i * 4); // MSAT说SecID=id的sector是用来存放SAT的
    if (i < (int)_csectFat) {
      int val = get32(_sat, id * 4); // SAT中SecID=id的sector的表记了的值，应该是-3
      if (val != -3) {
        fprintf(stderr, "Warning: SecId=%d used for SAT, but SAT[%d]=%d\n", id, id, val);
        fflush(stderr);
      }
    } else {
      if (id != -1) {
        fprintf(stderr, "Warning: _csectFat = %u, _msat[%d] = %d\n", _csectFat, i, id);
        fflush(stderr);
      }
    }
  }
#endif
}
void Storage::_initDIR() {
  SECT _sectDirStart = get32(_buf, 0x030);
  std::vector<uint8_t> str = __read_stream(_sectDirStart, _sz, _sat, _buf);
  _dir.clear();
  for (int i = 0; i < (int)str.size(); i += 128) {
    DirEntry d;
    for (int j = 0; j < 32; ++j) d.name[j] = str[i + j * 2];
    d.type = (STGTY)str[i + 66];
    d.secID = get32(str, i + 116);
    d.streamSize = get32(str, i + 120);
    if (d.type != 0) _dir.push_back(d);
  }
}
void Storage::_initSSAT() {
  SECT _sectMiniFatStart = get32(_buf, 0x03C);
  //FSINDEX _csectMiniFat = get32(_buf, 0x040);
  _ssat = __read_stream(_sectMiniFatStart, _sz, _sat, _buf);
  _miniBuf = __read_stream(_dir[0].secID, _sz, _sat, _buf);
}

std::vector<uint8_t> Storage::__read_stream(int i, int sz, const std::vector<uint8_t> &sat, const std::vector<uint8_t> &buf, int _offset) {
  std::vector<uint8_t> str;
  for (; i >= 0; i = get32(sat, 4 * i)) {
    for (int j = 0; j < sz && _offset + i * sz + j < (int)buf.size(); ++j) {
      str.push_back(buf[_offset + i * sz + j]);
    }
  }
  return str;
}

bool Storage::readStream(const char *name, __out std::vector<uint8_t> &str) {
  for (int i = 0; i < (int)_dir.size(); ++i) {
    if (strcmp(_dir[i].name, name) != 0) continue;
    int id = _dir[i].secID;
    str = _dir[i].streamSize > _ulMiniSectorCutoff ? __read_stream(id, _sz, _sat, _buf) : __read_stream(id, _ssz, _ssat, _miniBuf, 0);
    return true;
  }
  return false;
}
