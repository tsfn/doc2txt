// #define PARSE_TO_XML

#include "Storage.hpp"
#include "doc_text.hpp"
#include "doc_image.hpp"
#include "doc_format.hpp"
#include "doc_property.hpp"
#include "parse_doc.hpp"
#include <cstdio>

#ifndef PARSE_TO_XML
bool parse_doc(const char *doc_file_path, const char *text_file_path, const char *image_dir_path) {
  FILE *doc_file = fopen(doc_file_path, "rb");
  if (doc_file == NULL) {
    return false;
  }

  /* 初始化流 */
  Storage storage;
  if (!storage.init(doc_file)) {
    fclose(doc_file);
    return false;
  }
  fclose(doc_file);

  /* 提取文本 */
  FILE *text_file = fopen(text_file_path, "wb");
  if (text_file == NULL) {
    return false;
  }
  doc_text_file(storage, text_file);
  fclose(text_file);

  /* 提取图片 */
  doc_image(storage, image_dir_path);

  /* 分析格式 */
  doc_format(storage);

  /* 标题、作者等信息 */
  doc_property(storage);
  return true;
}

#else

CutilXml *parse_doc(const char *doc_file_path, const char *image_dir_path)
{
  FILE *doc_file = fopen(doc_file_path, "rb");
  if (doc_file == NULL) {
    return false;
  }

  /* 初始化流 */
  Storage storage;
  if (!storage.init(doc_file)) {
    fclose(doc_file);
    return false;
  }
  fclose(doc_file);

  CutilXml *xml = cutil_xml_new_fre("Scan", "")

  /* 提取文本 */
  uchar *text = doc_text(storage);
  if (text != NULL) {
    cutil_xml_insert(xml, text, "Scan/Content");
    free(text);
  }
}
#endif
