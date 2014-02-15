#include "ole.h"

using namespace tsfn;

std::vector<uint8_t> GetPieceTable(Storage &s, const std::vector<uint8_t> &wds) {
	uint32_t ClxOffset = get32(wds, 0x01A2);
	uint32_t ClxLength = get32(wds, 0x01A6);
	std::vector<uint8_t> ts;
	s.readStream(((get16(wds, 10) & 0x0200) == 0x0200) ? "1Table" : "0Table", ts);
	std::vector<uint8_t> clx(ClxLength);
	for (int i = 0; i < (int)ClxLength; ++i) {
		clx[i] = ts[ClxOffset + i];
	}
	std::vector<uint8_t> pt;//Piece Table
	for (int pos = 0; ;) {
		uint8_t type = clx[pos];
		if (type == 2) {
			int len = get32(clx, pos + 1);
			pt.clear();
			for (int i = 0; i < len; ++i) {
				pt.push_back(clx[pos + 5 + i]);
			}
			break;
		} else if (type == 1) {
			pos += 3 + get16(clx, pos + 1);
		} else {
			assert(false);
		}
	}
	return pt;
}

void WriteFromPieceTable(FILE *file, const std::vector<uint8_t> &pieceTable, const std::vector<uint8_t> &wds) {
	fputc(0xFF, file);
	fputc(0xFE, file); // BOM: 0xFFFE, Little-Endian
	int lcbPieceTable = pieceTable.size();
	int piece_count = (lcbPieceTable - 4) / 12;
	for (int i = 0; i < piece_count; ++i) {
		uint32_t cp_start = get32(pieceTable, i * 4);
		uint32_t cp_end = get32(pieceTable, (i + 1) * 4);
		uint32_t fc = get32(pieceTable, (piece_count + 1) * 4 + i * 8 + 2);
		bool isUnicode = (fc & (1U << 30)) == 0;
		fprintf(stderr, "piece[%u...%u], isUnicode=%d\n", cp_start, cp_end, isUnicode);
		if (!isUnicode) fc = (fc & ~(1U << 30)) / 2;
		for (int j = 0; j < (cp_end - cp_start) * (isUnicode ? 2 : 1); ++j) {
			fputc(wds[fc + j], file);
			//fprintf(stderr, "%02X\n", wds[fc + j]);
		}
	}
}

// in Unicode(UTF-16LE)
void WriteStream(FILE *file, const std::vector<uint8_t> &str) {
	// BOM: 0xFFFE, Little-Endian
	fputc(0xFF, file);
	fputc(0xFE, file);
	for (std::vector<uint8_t>::const_iterator iter = str.begin(); iter != str.end(); ++iter) {
		fputc(*iter, file);
	}
}


int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "at least 2 files: arg[1]=input file, arg[2]=output file\n");
    return 1;
  }

	FILE *file = fopen(argv[1], "rb");
	Storage s(file);
	fclose(file);

	//s.print_dir();

	std::vector<uint8_t> str;
	FILE *tmp_file = fopen(argv[2], "wb");
	if (s.readStream("WordDocument", str)) {
		std::vector<uint8_t> pt = GetPieceTable(s, str); //Piece Table
		WriteFromPieceTable(tmp_file, pt, str);
	} else if (s.readStream("PowerPoint Document", str)) {
		WriteStream(tmp_file, str);
	} else if (s.readStream("Workbook", str)) {
		WriteStream(tmp_file, str);
	}
	fclose(tmp_file);

	//system("pause");
	return 0;
}
