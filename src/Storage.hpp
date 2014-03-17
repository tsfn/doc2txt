/*
 * 注意用storage.stream(name, &stream, &size)得到流的时候不要手动free(stream)
 * Storage的析构函数~Storage()会自动free掉Storage中所有分配的内存
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <cstdio>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

class Storage {
public:
  bool init(FILE *doc_file);
  bool stream(const char *name, const uchar **stream, uint *size) const;
private:
  uint stream_num;
  uchar **stream_name;
  uchar **stream_table;
  uint *stream_sizes;
public:
  Storage();
  ~Storage();
};
#endif
