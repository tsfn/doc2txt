通过分析二进制的OLE结构得到doc中的WordDocument Stream，Table Stream等部分，
然后用其中的某些字段得到文本和格式信息。

### Compilation

`$ make`

### Encoding

the extracted text is encoded in UTF-16.
ANSI is not supported.
