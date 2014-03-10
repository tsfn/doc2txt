## 提取图片

#### 提取OfficeArtContent

根据FibRgFcLcb97中的fcDggInfo和lcbDggInfo从Table Stream中得到OfficeArtContent结构。

    FibRgFcLcb97 {
        ...
        fcDggInfo (4 bytes); // OfficeArtContent在Table Stream中的偏移
        lcbDggInfo (4 bytes); // OfficeArtContent在Table Stream中的字节数
        ...
    }

得到OfficeArtContent的大致过程：

    uchar *word_document_stream = storage.read_stream("WordDocument");
    uchar *table_stream = get_table_stream(storage); // 得到table stream的过程和之前的一样
    uint fcDggInfo = *(uint *)(word_document_stream + 554); // 偏移位置由计算得到
    uint lcbDggInfo = *(uint *)(word_document_stream + 558);
    uchar *OfficeArtContent = table_stream + fcDggInfo;

#### 解析OfficeArtContent结构

跳过OfficeArtContent中的开头部分，得到blipStore：

    OfficeArtContent {
        *DrawingGroupData: OfficeArtDggContainer (variable) {
            rh: OfficeArtRecordHeader (8 bytes);
            drawingGroup: OfficeArtFDGGBlock (variable) {
                rh: OfficeArtRecordHeader (8 bytes);
                head: OfficeArtFDGG (16 bytes);
                Rgidcl: OfficeArtIDCL[];
            }
            blipStore: OfficeArtBStoreContainer {
                // specifies the container for all the BLIPs
                rh: OfficeArtRecordHeader (8 bytes);
                rgfb: OfficeArtBStoreContainerFileBlock[] {
                    rh.recType=0xF007 -> OfficeArtFBSE {
                        rh: OfficeArtRecordHeader (8 bytes);
                        btWin32: MSOBLIPTYPE (1 byte);
                        btMaxOS: MSOBLIPTYPE (1 byte);
                        rgbUid: MD4Diget (16 bytes);
                        tag: ushort (2 bytes); // 0xFF表示外部文件
                        size: uint (4 bytes); // size of BLIP (in bytes)
                        cRef: uint (4 bytes);
                        // specifies the number of references to the BLIP.
                        // 0x00 specifies an empty slot in the
                        // OfficeArtBStoreContainer record
                        foDelay: MSOFO (4 bytes);
                        // OfficeArtBStoreDelay在delay stream中的偏移量
                        // OfficeArtBStoreDelay就是一个
                        //      OfficeArtBStoreContainerFileBlock的数组
                        // 0xFFFFFFFF表示文件不在delay stream中，此时cRef=0x00
                        unused1: uchar (1 byte);
                        cbName: uchar (1 byte);
                        unused2: uchar (1 byte);
                        unused3: uchar (1 byte);
                        nameData: wchar_t[] (variable); // BLIP的名字
                        embeddedBlip: OfficeArtBlip (variable);
                    }
                    rh.recType=0xF018~0xF117 -> OfficeArtBlip {
                        rh.recType=0xF01A -> OfficeArtBlipEMF;
                        rh.recType=0xF01B -> OfficeArtBlipWMF;
                        rh.recType=0xF01C -> OfficeArtBlipPICT;
                        rh.recType=0xF01D -> OfficeArtBlipJPEG;
                        rh.recType=0xF01E -> OfficeArtBlipPNG;
                        rh.recType=0xF01F -> OfficeArtBlipDIB;
                        rh.recType=0xF029 -> OfficeArtBlipTIFF;
                        rh.recType=0xF02A -> OfficeArtBlipJPEG;
                    }
                }
            }
            ...
        }
        Drawings: OfficeArtWordDrawing[] {
            dgglbl (1 byte);
            // 0x00表示container在Main Document中
            // 0x01表示container在Header Document中
            container: OfficeArtDgContainer (variable) { /* 在MS-ODRAW中定义 */
                rh: OfficeArtRecordHeader (8 bytes);
                drawingData: OfficeArtFDG (16 bytes);
                regroupIterms: OfficeArtFRITContainer (variable);
                groupShape: OfficeArtSpgrContainer (variable) {
                    rh: OfficeArtRecordHeader (8 bytes);
                    rgfb: OfficeArtSpgrContainerFileBlock (variable) {
                        -> OfficeArtSpContainer;
                        -> OfficeArtSpgrContainer;
                    }
                }
                shape: OfficeArtSpContainer (variable) {
                    rh: OfficeArtRecordHeader (8 bytes);
                    shapeGroup: OfficeArtFSPGR (24 bytes);
                    shapeProp: OfficeArtFSP (16 bytes);
                    deletedShape: OfficeArtFPSPL (12 bytes);
                    shapePrimaryOptions: OfficeArtFOPT (variable);
                    shapeSecondaryOptions1: OfficeArtSecondaryFOPT (variable);
                    shapeTertiaryOptions1: OfficeArtTertiaryFOPT (variable);
                    childAnchor: OfficeArtChildAnchor (24 bytes);
                    clientAnchor: OfficeArtClientAnchor (variable);
                    clientData: OfficeArtClientData (variable);
                    clientTextbox: OfficeArtClientTextbox (variable);
                    shapeSecondaryOption2: OfficeArtSecondaryFOPT (variable);
                    shapeTertiaryOptions2: OfficeArtTertiaryFOPT (variable);
                }
                deletedShapes: OfficeArtSpgrContainerFileBlock (variable);
                solvers2: OfficeArtSolverContainer (variable);
            }
        }
    }

其中的各个rh字段都是以下相同的结构：

    OfficeArtRecordHeader {
        recVer (4 bits);
        recInstance (12 bits); 
        recType (2 bytes); // 0xF000~0xFFFF
        recLen (4 bytes); // 结构体的长度
    }

得到blipStore后得到rgfb

    uchar *blipStore = OfficeArtContent + 16 + *(uint *)(OfficeArtContent + 12);
    uchar *rgfb = blipStore + 8;

rgfb是个OfficeArtBStoreContainerFileBlock的数组，其中的每个元素指向了一个图片。

每个元素开头的rh结构中的recType等于0xF007时这个元素是一个OfficeArtFBSE结构，这时图片在WordDocument Stream中；否则它是一个OfficeArtBlip结构，这时图片内嵌在当前结构中。

    while (rgfb < blipStore + *(uint *)(blipStore + 4)) {
        ushort recType = *(ushort *)(rgfb + 2);
        uint recSize = *(uint *)(rgfb + 4);
        if (recType == 0xF007) {
            uint size = *(uint *)(rgfb + 28);
            uint foDelay = *(uint *)(rgfb + 36);
            storeBlipBlock(word_document_stream + foDelay, img_name, size);
        } else {
            storeBlipBlock(rgfb, img_name, recSize);
        }
    }

#### 解析OfficeArtBlip

这里的OfficeArtBlip可能是从WordDocument Stream中取得的，也可能是包含在OfficeArtContent的。

根据rh.recType的值确定图片的格式：

* rh.recType=0xF01A, OfficeArtBlip是个OfficeArtBlipEMF
* rh.recType=0xF01B, OfficeArtBlip是个OfficeArtBlipWMF
* rh.recType=0xF01C, OfficeArtBlip是个OfficeArtBlipPICT
* rh.recType=0xF01D, OfficeArtBlip是个OfficeArtBlipJPEG
* rh.recType=0xF01E, OfficeArtBlip是个OfficeArtBlipPNG
* rh.recType=0xF01F, OfficeArtBlip是个OfficeArtBlipDIB
* rh.recType=0xF029, OfficeArtBlip是个OfficeArtBlipTIFF
* rh.recType=0xF02A, OfficeArtBlip是个OfficeArtBlipJPEG

不同的格式的结构的头部长度不同，需要跳过的长度也不同。

    static bool storeBlipBlock(uchar *block, char *img_name, uint size) {
        ushort recInstance = *(ushort *)(block) >> 4;
        ushort recType = *(ushort *)(block + 2);

        char name[32];
        uchar *start = block;
        if (recType == 0xF01A) { // EMF
            sprintf(name, "%s.emf", img_name);
            start += 8 + 16 + (recInstance == 0x3D5 ? 16 : 0) + 34;
        } else if (recType == 0xF01B) { // WMF
            sprintf(name, "%s.wmf", img_name);
            start += 8 + 16 + (recInstance == 0x217 ? 16 : 0) + 34;
        } else if (recType == 0xF01C) { // PICT
            sprintf(name, "%s.pict", img_name);
            start += 8 + 16 + (recInstance == 0x543 ? 16 : 0) + 34;
        } else if (recType == 0xF01D || recType == 0xF02A) { // JPEG
            sprintf(name, "%s.jpeg", img_name);
            start += (recInstance == 0x46B || recInstance == 0x6E3 ? 41 : 25);
        } else if (recType == 0xF01E) { // PNG
            sprintf(name, "%s.png", img_name);
            start += 8 + 16 + (recInstance == 0x6E1 ? 16 : 0) + 1;
        } else if (recType == 0xF01F) { // DIB
            sprintf(name, "%s.dib", img_name);
            start += 8 + 16 + (recInstance == 0x7A9 ? 16 : 0) + 1;
        } else if (recType == 0xF029) { // TIFF
            sprintf(name, "%s.tiff", img_name);
            start += 8 + 16 + (recInstance == 0x6E5 ? 16 : 0) + 1;
        } else {
            return false;
        }

        FILE *file = fopen(name, "wb");
        for (uint i = 0; i < size; ++i) {
            fputc(start[i], file);
        }
        fclose(file);
        return true;
    }
