#include "Storage.hpp"
#include <set>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

Storage::Storage() {
  stream_num = 0U;
  stream_name = NULL;
  stream_table = NULL;
  stream_sizes = NULL;
}

Storage::~Storage() {
  for (uint i = 0; i < stream_num; ++i) {
    if (stream_name != NULL && stream_name[i] != NULL) {
      free(stream_name[i]);
      stream_name[i] = NULL;
    }
    if (stream_table != NULL && stream_table[i] != NULL) {
      free(stream_table[i]);
      stream_table[i] = NULL;
    }
  }
  stream_num = 0;
  if (stream_name != NULL) {
    free(stream_name);
    stream_name = NULL;
  }
  if (stream_table != NULL) {
    free(stream_table);
    stream_table = NULL;
  }
  if (stream_sizes != NULL) {
    free(stream_sizes);
    stream_sizes = NULL;
  }
}

static bool read_buf(FILE *doc_file, uchar **buf, uint *buf_size) {
  fseek(doc_file, 0, SEEK_END);
  *buf_size = ftell(doc_file);
  if (*buf_size < 512) {
    return false;
  }
  *buf_size = ((*buf_size - 512) + 511) / 512 * 512 + 512;
  if ((*buf = (uchar *)malloc(*buf_size)) == NULL) {
    return false;
  }
  fseek(doc_file, 0, SEEK_SET);
  int ch, i = 0;
  while ((ch = fgetc(doc_file)) != EOF) {
    *(*buf + i++) = (uchar)ch;
  }
  return true;
}

static bool read_msat(uchar *buf, uint buf_size, uchar **msat, uint *msat_size) {
  set<uint> vis;
  uint num_sec = 0, id = *(uint *)(buf + 0x44);
  for (; id < buf_size / 512 - 1; id = *(uint *)(buf + 512 + 512 * id + 508)) {
    if (vis.count(id)) {
      return false;
    }
    vis.insert(id);
    num_sec += 1;
  }
  *msat_size = 109 * 4 + num_sec * 512;
  if ((*msat = (uchar *)malloc(*msat_size)) == NULL) {
    return false;
  }
  memcpy(*msat, buf + 0x4C, 109 * 4);
  num_sec = 0;
  id = *(int *)(buf + 0x44);
  for (; id < buf_size / 512 - 1; id = *(int *)(buf + 512 + 512 * id + 508)) {
    memcpy(*msat + 436 + num_sec++ * 508, buf + 512 + 512 * id, 508);
  }
  return true;
}

static bool read_sat(uchar *buf, uint buf_size, uchar *msat, uint msat_size,
    uchar **sat, uint *sat_size) {
  *sat_size = 0;
  for (uint i = 0; i < msat_size / 4; ++i) {
    uint id = *(uint *)(msat + i * 4);
    if (id > buf_size / 512 - 1) break;
    *sat_size += 1;
  }
  *sat_size *= 512;
  if ((*sat = (uchar *)malloc(*sat_size)) == NULL) {
    return false;
  }
  for (uint i = 0; i < msat_size / 4; ++i) {
    uint id = *(uint *)(msat + i * 4);
    if (id > buf_size / 512 - 1) break;
    memcpy(*sat + i * 512, buf + 512 + id * 512, 512);
  }
  return true;
}

static bool read_stream(uint start, uint sec_sz, uchar *sat, uint sat_size,
    uchar *buf, uint buf_size, uchar **stream, uint *sz) {
  uint sec_num = 0;
  set<uint> vis;
  for (uint id = start; id < buf_size / sec_sz; id = *(uint *)(sat + id * 4)) {
    if (vis.count(id)) return false;
    vis.insert(id);
    ++sec_num;
  }
  *sz = sec_num * sec_sz;
  *stream = (uchar *)malloc(*sz);
  sec_num = 0;
  for (uint id = start; id < buf_size / sec_sz; id = *(uint *)(sat + id * 4)) {
    memcpy(*stream + sec_num++ * sec_sz, buf + id * sec_sz, sec_sz);
  }
  return true;
}

bool Storage::init(FILE *doc_file) {
  if (doc_file == NULL) {
    return false;
  }

  uchar *buf, *mbuf, *msat, *sat, *ssat, *dir_stream;
  uint buf_size, mbuf_size, msat_size, sat_size, ssat_size, dir_stream_size;

  if (!read_buf(doc_file, &buf, &buf_size)) return false;
  if (!read_msat(buf, buf_size, &msat, &msat_size)) return false;
  if (!read_sat(buf, buf_size, msat, msat_size, &sat, &sat_size)) return false;
  free(msat);

  /* Directory Stream */
  if (!read_stream(*(uint *)(buf + 0x30), 512, sat, sat_size,
        buf + 512, buf_size - 512, &dir_stream, &dir_stream_size)) {
    return false;
  }
  if (dir_stream_size < 128) return false;

  /* short-sector Stream's mbuf and ssat */
  if (!read_stream(*(uint *)(dir_stream + 0x74), 512, sat, sat_size,
        buf + 512, buf_size - 512, &mbuf, &mbuf_size)) {
    return false;
  }
  if (!read_stream(*(uint *)(buf + 0x3C), 512, sat, sat_size,
        buf + 512, buf_size - 512, &ssat, &ssat_size)) {
    return false;
  }

  /* all streams */
  uint _ulMiniSectorCutoff = *(uint *)(buf + 0x38);

  stream_num = dir_stream_size / 128 - 1;
  stream_name = (uchar **)malloc(stream_num * sizeof(uchar *));
  stream_table = (uchar **)malloc(stream_num * sizeof(uchar *));
  stream_sizes = (uint *)malloc(stream_num * sizeof(uint));

  memset(stream_name, 0, stream_num * sizeof(uchar *));
  memset(stream_table, 0, stream_num * sizeof(uchar *));
  memset(stream_sizes, 0, stream_num * sizeof(uint));

  for (uint i = 0; i < stream_num; ++i) {
    uchar *dir_entry = dir_stream + i * 128;
    stream_name[i] = (uchar *)malloc(64);
    memcpy(stream_name[i], dir_entry, 64);
    if (i == 0) continue;
    uint id = *(uint *)(dir_entry + 0x74);
    stream_sizes[i] = *(uint *)(dir_entry + 0x78);
    /*
    for (uint j = 0; j < 32; ++j) {
      printf("%c", stream_name[i][j * 2]);
    }
    printf("-> sz=%u, id=%u\n", stream_sizes[i], id);
    */
    if (stream_sizes[i] >= _ulMiniSectorCutoff) {
      if (!read_stream(id, 512, sat, sat_size,
            buf + 512, buf_size - 512, &stream_table[i], &stream_sizes[i])) {
        return false;
      }
    } else {
      if (!read_stream(id, 64, ssat, ssat_size,
            mbuf, mbuf_size, &stream_table[i], &stream_sizes[i])) {
        return false;
      }
    }
  }

  free(buf);
  free(mbuf);
  free(sat);
  free(ssat);
  free(dir_stream);
  return true;
}

bool Storage::stream(const char *name, const uchar **stream, uint *size) const {
  for (uint i = 0; i < stream_num; ++i) {
    bool match = true;
    for (uint j = 0; name[j] != '\0'; ++j) {
      if (name[j] != stream_name[i][j * 2]) {
        match = false;
        break;
      }
    }
    if (match) {
      *stream = stream_table[i];
      *size = stream_sizes[i];
      return true;
    }
  }
  return false;
}
