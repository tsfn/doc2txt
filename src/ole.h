#ifndef OLE_H
#define OLE_H

#include <cstdio>
#include <cassert>
#include <cstring>

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

typedef enum tagSTGTY {
	STGTY_INVALID	 = 0,
	STGTY_STORAGE	 = 1,
	STGTY_STREAM	 = 2,
	STGTY_LOCKBYTES	 = 3,
	STGTY_PROPERTY	 = 4,
	STGTY_ROOT		 = 5,
} STGTY;

typedef enum tagDECOLOR {
	DE_RED = 0,
	DE_BLACK = 1,
} DECOLOR;

typedef struct tagFILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, TIME_T;

class DirEntry {
public:
	char name[64];
	uint32_t secID;
	uint32_t streamSize;
	STGTY type;
};

class Storage {
private:
	std::vector<uint8_t> _buf;
	std::vector<uint8_t> _miniBuf;
	std::vector<uint8_t> _sat;
	std::vector<DirEntry> _dir;
	std::vector<uint8_t> _ssat;

private:
	int _sz; // size of a sector
	int _ssz; // size of a short-sector

private:
	//static const uint8_t _abSig[8] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
	//CLSID _clid;
	//USHORT _uMinorVersion;
	//USHORT _uDllVersion;
	//USHORT _uByteOrder;
	USHORT _uSectorShift; //*
	USHORT _uMiniSectorShift; //*
	//USHORT _usReserved;
	//ULONG _ulReserved1;
	//ULONG _ulReserved2;
	FSINDEX _csectFat; // SAT用掉了几个sector
	//SECT _sectDirStart;
	//DFSIGNATURE _signature;
	ULONG _ulMiniSectorCutoff;

public: // debug
	void print_dir() {
		for (int i = 0; i < (int)_dir.size(); ++i) {
			printf("%s\n", _dir[i].name);
		}
	}

private:
	void _init();
	void _initSAT();
	void _initDIR();
	void _initSSAT();
	std::vector<uint8_t> __read_stream(int sec_id, int sec_sz, const std::vector<uint8_t> &sat, const std::vector<uint8_t> &buf, int _offset = 512);

public:
	Storage(FILE *file);
	Storage(const std::vector<uint8_t> &arg_buf);
	Storage(uint8_t *arg_buf, uint32_t length);

	bool readStream(const char *name, __out std::vector<uint8_t> &stream);
};

namespace tsfn {
	uint16_t get16(const std::vector<uint8_t> &buf, int offset);
	uint32_t get32(const std::vector<uint8_t> &buf, int offset);
	void output_hexadecimal(uint8_t *buf, int size, std::string name="", FILE *file=stderr);
	void output_hexadecimal(const std::vector<uint8_t> &buf, std::string name="", FILE *file=stderr);
}
#endif
