## 从doc文件中提取格式信息

MS-Word中的格式（format）有文字格式、段落格式、图片格式、表单格式等。加粗/斜体和字体大小等属于文字格式，居中/右对齐等属于段落格式。

### WordDocument Stream

根据WordDocument Stream中的`fcPlcfBteChpx`字段和`fcPlcfBtePapx`字段，分别从Table Stream找到PlcBteChpx结构和PlcBtePapx结构。文字格式用到PlcfBteChpx结构，段落格式用到PlcBtePapx结构。

WordDocument Stream中的相关结构：

    WordDocument Stream {
        Fib: FIB (variable) {
            base: FibBas (32 bytes);
            ... // 其他无关字段
            cbRgFcLcb: ushort (2 bytes); // fibRgFcLcbBlob占用多少字节
            fibRgFcLcbBlob: FibRgFcLcb (variable) {
                fibRgFcLcb97: FibRgFcLcb97 (0x00C1 bytes) {
                    ... // 其他无关字段
                    fcPlcfBteChpx: uint; // PlcfBteChpx在Table Stream中的位置
                    lcbPlcfBteChpx: uint; // PlcfBteChpx在Table Stream中占用多少字节
                    fcPlcfBtePapx: uint; // PlcBtePapx在Table Stream中的位置
                    lcbPlcfBtePapx: uint; // PlcBtePapx在Table Stream中占用多少字节
                    ... // 其他无关字段
                }
                ... // 其他无关字段
            }
            ... // 其他无关字段
        }
        ... // 其他无关字段
    }

需要用到这几个字段：

    uchar *wds = read_stream(&st, "WordDocument");
    uint fcPlcfBteChpx = *(uint *)(wds + 250);
    uint lcbPlcfBteChpx = *(uint *)(wds + 254);
    uint fcPlcfBtePapx = *(uint *)(wds + 258);
    uint lcbPlcfBtePapx = *(uint *)(wds + 262);

### PlcBteChpx和PlcBtePapx

PlcBteChpx和PlcBtePapx都在Table Stream中，需要根据它们找到在WordDocument Stream中的多个ChpxFkp和PapxFkp结构。

PlcBteChpx和PlcBtePapx中的相关信息：

    PlcBteChpx {
        aFC: FC[]; // 一个uint的数组，表示取到哪些文本，没有用到
        aPnBteChpx: PnFkpChpx[] {
            PnFkpChpx: (4 bytes) {
                22bits pn; // ChpxFkp在WordDocument Stream中的偏移
                10bits unused; // 无关信息
            }
        }
    }

    PlcBteChpx {
        aFC: FC[]; // 一个uint的数组，表示取到哪些文本，没有用到
        aPnBtePapx: PnFkpPapx[] {
            PnFkpPapx: (4 bytes) {
                22bits pn; // PapxFkp在WordDocument Stream中的偏移
                10bits unused; // 无关信息
            }
        }
    }

先得到Table Stream：

    uint16_t flag1 = *(ushort *)(wds + 0xA);
    bool is1Table = (flag1 & 0x0200) == 0x0200;
    uchar *table_stream = read_stream(&st, is1Table ? "1Table" : "0Table");

根据微软的文档，需要根据`lcbPlcfBteChpx`计算PlcBteChpx中数组的元素个数`n = (lcbPlcfBteChpx - 4) / 8`。然后从Table Stream得到每个PnFkpChpx和PnFkpPapx，然后依次处理它们：

    uint i, n;
    uchar *PlcBteChpx = table_stream + fcPlcfBteChpx;
    uchar *PlcBtePapx = table_stream + fcPlcfBtePapx;

    n = (lcbPlcfBteChpx - 4) / 8;
    for (i = 0; i < n; i += 1) {
        uint PnFkpChpx = *(uint *)(PlcBteChpx + 4 * (n + 1 + i));
        uchar *ChpxFkp = wds + PnBteChpx * 512;
        // 得到ChpxFkp，对它进行处理
    }

    n = (lcbPlcfBtePapx - 4) / 8;
    for (i = 0; i < n; i += 1) {
        uint PnFkpPapx = *(uint *)(PlcBtePapx + 4 * (n + 1 + i));
        uchar *PapxFkp = wds + PnBtePapx * 512;
        // 得到PapxFkp，对它进行处理
    }


### ChpxFkp

ChpxFkp的大小固定是`512`字节，它的最后一个字节表示一个无符号整型数`crun`，`crun`的取值范围必须在`0x01`和`0x65`之间。

    ChpxFkp (512 bytes) {
        uint rgfc[]; // crun + 1个元素的uint数组，表示WordDocument Stream中的偏移量
        uchar rgb[] { // curn个元素的uchar数组
            uchar bOffset; // bOffset*2是Chpx在ChpxFkp中的位移
        }
        Chpx[] {
            cb (1byte);
            grpprl: Prl[] {
                sprm (2 bytes);
                operand (sprm.spra .bytes);
            }
        }
        crun(last 1byte); // 0x01 <= crun <= 0x65
    }

其中rgfc有`crun+1`个元素，rgb有`crun`个元素。

相邻的两个rgfc指定了一段文本，对应的rgbx指定了一个Chpx，这个Chpx中的`grpprl`部分描述了这段文本的格式。如果rgb等于`0`，则没有Chpx描述这段文字。

    uchar crun = *(uchar *)(ChpxFkp + 511);
    for (uchar i = 0; i < crun; i += 1) {
        uint rgfc_start = *(uint *)(ChpxFkp + 4 * i);
        uint rgfc_end = *(uint *)(ChpxFkp + 4 * i + 4);
        uchar bOffset = *(uchar *)(ChpxFkp + 4 * (crun + 1) + i);
        uchar *Chpx = ChpxFkp + 2 * bOffset;
        uint cb = *(uchar *)ChpxFkp;
        uchar *grpprl = Chpx + 1;
        // 根据cb和grpprl处理grpprl结构
    }


### PapxFkp

ChpxFkp的大小固定是`512`字节，它的最后一个字节表示一个无符号整型数`cpara`，`cpara`的取值范围必须在`0x01`和`0x1D`之间。

    PapxFkp (512bytes) {
        uint rgfc[]; // cpara + 1个元素的uint数组，表示WordDocument Stream中的偏移量
        BxPap rgbx[] { // cpara个元素的BxPap数组
            bOffset (1 byte); // bOffset*2是PapxInFkp在PapxFkp中的位移
            reserved (12 bytes);
        }
        PapxInFkp[] {
            // if cb!=0, grpprlInPapx没有cb'部分，它的大小为2*cb-1字节
            // if cb==0, grpprlInPapx有cb'部分，它的大小为cb'*2确定
            cb (1byte);
            grpprlInPapx (variable) {
                cb' (1byte, optional);
                GrpPrlAndIstd (variable) {
                    istd (2 bytes); // 指定一个style，在这里用不到
                    grpprl; // 和ChpxFkp中的grpprl的类型一样，处理方法也一样
                }
            }
        }
        ..
        cpara(last 1byte); // 0x01 <= cpara <= 0x1D
    }

其中rgfc有`cpara+1`个元素，rgb有`cpara`个元素。

相邻的两个rgfc指定了一段文本，对应的rgbx指定了一个PapxInFkp，其中的grpprlInPapx中的GrpPrlAndIstd中的grpprl描述了这段文本的段落格式。

这里的grpprl和在ChpxFkp中的grpprl结构一样，处理方式也类似，但是它在grpprlInPapx结构中的grpprlInPapx中，因此需要特殊处理`cb`字段。

    uchar cpara = *(uchar *)(PapxFkp + 511);
    for (uchar i = 0; i < cpara; ++i) {
        uint rgfc_start = *(uint *)(PapxFkp + 4 * i);
        uint rgfc_end = *(uint *)(PapxFkp + 4 * i + 4);
        uchar bOffset = *(uchar *)(PapxFkp + 4 * (cpara + 1) + 13 * i);
        uchar cb = *(uchar *)(PapxFkp + bOffset * 2);
        uint lcbGrpPrlAndIstd = 2 * cb - 1;
        uchar *GrpPrlAndIstd = PapxFkp + bOffset * 2 + 1;
        if (cb == 0) {
            lcbGrpPrlAndIstd = *GrpPrlAndIstd * 2;
            GrpPrlAndIstd += 1;
        }
        cb = lcbGrpPrlAndIstd - 2;
        uchar *grpprl = GrpPrlAndIstd + 2;
        // 根据cb和grpprl处理grpprl结构
    }


### grpprl

grpprl是一个Prl的数组，每个Prl的结构如下：

    Prl {
        sprm (2 bytes) { // 指定格式的类型
            ispmd (9 bits);
            // 和fSpec一起指定某种格式，具体值指哪种格式需要查表

            fSpec (1 bits);

            sgc (3 bits);
            // 格式类型
            // sgc = 1 时 sprm是一个段落格式（paragraph property）
            // sgc = 2 时 sprm是一个文字格式（character property）
            // sgc = 3 时 sprm是一个图片格式（picture property）
            // sgc = 4 时 sprm是一个section property
            // sgc = 5 时 sprm是一个table property

            spra (3 bits);
            // 参数operand的大小，
            // spra = 0 时 operand占1字节
            // spra = 1 时 operand占1字节
            // spra = 2 时 operand占2字节
            // spra = 3 时 operand占4字节
            // spra = 4 时 operand占2字节
            // spra = 5 时 operand占2字节
            // spra = 6 时 operand是变长的
            // spra = 7 时 operand占3字节
        }

        operand (variable); // 格式的参数
    }

每个Prl包括sprm部分和operand部分，sprm表示某种格式，operand表示这种格式的参数。grpprl中依次存放多个Prl，由于Prl是变长的，提取过程需要一定的计算。

    static const int operand_size[8] = { 1, 1, 2, 4, 2, 2, -1, 3, };
    uint i = 0;
    while (i < cb) {
        ushort sprm = *(ushort *)(grpprl + i);
        uint ispmd = sprm & 0x01FF;
        uint f = (sprm / 512) & 0x0001;
        uint sgc = (sprm / 1024) & 0x0007;
        uint spra = sprm / 8192;
        uint operand;
        i += 2;
        if (spra == 6) {
            i += 1 + *(uchar *)(grpprl + i);
        } else {
            if (operand_size[spra] == 1) {
                operand = *(uchar *)(grpprl + i);
            } else if (operand_size[spra] == 2) {
                operand = *(ushort *)(grpprl + i);
            } else if (operand_size[spra] == 3) {
                operand = *(uint *)(grpprl + i) & 0x00FFFFFF;
            } else if (operand_size[spra] == 4) {
                operand = *(uint *)(grpprl + i);
            } else {
                operand = -1;
            }
            i += operand_size[spra];
        }
        // 得到一个sprm和相应的operand
    }

#### 用到的sprm的取值和参数

* sprm = 0x4A43 表示文字的大小
    * operand 是一个ushort类整数，以半角为单位，值一定要在2到3276之间，默认值是20。
* sprm = 0x0835 表示文字是否加粗
    * operand = 0 不加粗
    * operand = 1 加粗
* sprm = 0x0836 表示文字是否是斜体
    * operand = 0 不是斜体
    * operand = 1 斜体
* sprm = 0x2A3E 表示文字是否有下划线
    * operand = 0 没有下划线
    * operand = 1 有下划线
* sprm = 0x2403 表示段落的物理位置的对齐方式
    * operand = 0 左对齐
    * operand = 1 居中
    * operand = 2 右对齐
    * operand = 3 同时左右对齐
    * operand = 4 同时左右对齐
    * operand = 5 同时左右对齐
* sprm = 0x2461 表示段落的逻辑位置的对齐方式
    * operand = 0 左对齐
    * operand = 1 居中
    * operand = 2 右对齐
    * operand = 3 同时左右对齐
    * ...

