#include "Storage.hpp"
#include "doc_text.hpp"
#include "doc_image.hpp"
#include "parse_doc.hpp"
#include <cstdio>

bool parse_doc(const char *doc_file_path, const char *text_file_path, const char *image_dir_path) {
  FILE *doc_file = fopen(doc_file_path, "rb");
  if (doc_file == NULL) {
    return false;
  }

  /* 初始化流 */
  Storage storage;
  if (!storage.init(doc_file)) {
    return false;
  }
  fclose(doc_file);

  /* 提取文本 */
  FILE *text_file = fopen(text_file_path, "wb");
  if (text_file == NULL) {
    return false;
  }
  if (!doc_text(storage, text_file)) {
    return false;
  }
  fclose(text_file);

  return true;
}
