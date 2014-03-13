#include "parse_doc.hpp"
#include <cstdio>

int main() {
  if (!parse_doc("dat/111.doc", "output.txt", "img")) {
    printf("error\n");
  }
  return 0;
}
