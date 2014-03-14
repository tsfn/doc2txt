#include <cstdio>
#include "parse_doc.hpp"

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("usage: %s <doc file> <text file> <image directory>\n", argv[0]);
    return 1;
  }

  if (!parse_doc(argv[1], argv[2], argv[3])) {
    printf("error\n");
  }
  return 0;
}
