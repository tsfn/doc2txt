#ifndef PARSE_TO_XML
bool parse_doc(const char *doc_file_path, const char *text_file_path, const char *image_dir_path);
#else
CutilXml *parse_doc(const char *doc_file_path, const char *image_dir_path);
#endif
