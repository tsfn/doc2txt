# doc文件存储格式

[维基百科](http://en.wikipedia.org/wiki/DOC_%28computing%29)
[MSDN文档](http://msdn.microsoft.com/en-us/library/dd904907)

doc文件是一种二进制复合文件（Binary Compound File），其中存储了若干个流（stream），一个流就是一串连续的字节，一个流可能是表示文件内部结构的数据、可能是用户数据、可能是一个二进制文件、或文本文件等。

每个流都有一个名字，用一个字符数组表示，doc文件的文本保存在其中一个名字叫*WordDocument*的流当中。为了通过名字取出某个流，我们需要分析二进制复合文件的内部结构。


### doc文件的基本结构：

整个文件包括一个文件头（compound document header），然后跟着一系列的区（sector）。
文件头的大小是固定512字节的，结构定义如下：

    typedef unsigned char uchar;		// 1 byte
    typedef unsigned short ushort;	// 2 bytes
    typedef unsigned int uint;		// 4 bytes
    struct StructuredStorageHeader {
      uchar _abSig[8]; // [0x00] 一定是{0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
      uchar _clid[16]; // [0x08]16个字节，id号
      ushort _uMinorVersion; // [0x18]次版本号
      ushort _uDllVersion; // [0x1A]主版本号
      ushort _uByteOrder; // [0x1C]只能是0xFFFE，表示小端存储（Little-Endian）
      ushort _uSectorShift; // *[0x1E] 表示每个sector区的大小是(1<<_uSectorShift)字节
      ushort _uShortSectorShift; // *[0x20] 表示每个short_sector区的大小是(1<<_uShortSectorShift)字节
      ushort _usReseverd; // [0x022]
      uint _usReseverd1; // [0x24]
      uint _usReseverd2; // [0x28]
      uint _csectFat; // *[0x2C] SAT占用了几个sector
      uint _sectDirStart; // *[0x30] 目录流（Directory stream）占用的第一个sector区的SecID
      uint _signature; // [0x34] 一定都是0
      uint _ulMiniSectorCutoff; // *[0x38] short_sector的最大大小，一般是4096字节
      uint _sectMiniFatStart; // *[0x3C] SSAT占用的第一个sector的SecID
      uint _csectMiniFat; // *[0x40] SSAT占用了多少个sector
      uint _sectDifStart; // *[0x44] MSAT占用的第一个sector的SecID
      uint _csectDif; // *[0x48] MSAT占用了多少个sector
      uint _sectFat[109]; // *[0x4C] MSAT的第一个部分，包含109个SecID
    };

sector的大小由`header._uSectorShift`定义，所有sector的大小都是相等的。按照sector在文件中排列的顺序，每个sector有一个从0开始的标号，叫做这个sector的SecID。因此，给定一个SecID，这个SecID所表示的sector在文件中存放的位置是从`sec_pos(SecID) = 512+SecID*(1<<_uSectShift)`开始的，根据SecID提取sector的方法就是从这个位置开始复制出`(1<<_uSectorShift)`字节的内容出来。

![alt text](http://tsfn.ws/wp-content/uploads/2014/02/01-300x120.jpg "doc文件的基本结构包括一个header和若干sector")

为了得到一个流，我们需要提前知道这个流的占用的第一个sector的SecID、以及sector分配表（Sector Allocation Table，SAT）。


### SecID的取值：

  * >= 0 表示某个sector的ID
  * -1 （Free SecID），空sector，表示这个sector不属于任何流
  * -2 （End Of Chain SecID），表示结尾
  * -3 （SAT SecID），表示这个sector被用于存放sector分配表（sector allocation table）
  * -4 （MSAT SecID），表示这个sector被用于存放MSAT(master sector allocation table)


### SAT的用法：

SAT是一个由MSAT得到的特殊的数组，其中当前位置（数组下标）表示当前sector的`SecID`，而这个数组在这个位置的值指的是下一个sector的`SecID`。

![alt text](http://tsfn.ws/wp-content/uploads/2014/02/02.jpg "SAT的用法")

比如说我们已经知道了目录流（Directory Stream，用来存放目录的流）的占用的第一个sector的`SecID`是`0`，那么为了得到这个流，我们先从`sec_pos(0)`处取出`SecID`为`0`的sector，然后根据`SAT[0]=2`得知目录流的下一个sector的`SecID`是`2`、并从`sec_pos(2)`处取出`SecID`为`2`的sector，然后根据`SAT[2]=3`得知目录流的下一个sector的`SecID`是`3`、并从`sec_pos(3)`处取出`SecID`为`3`的sector，然后根据`SAT[3]=-2`得知目录流没有下一个sector了。像这样的`SecID`的序列被称为`SecID`链（SecID chain）。


### 如何得到MSAT和SAT：

为了得到SAT，我们需要先得到MSAT（Master Sector Allocation Table），MSAT是一个存放SecID的数组，这些SecID表示SAT用到了哪些sector。我们把SAT用到的sector叫做SatSector。

MSAT的前109个SecID存放在`header._sectFat`中，如果MSAT包含的`SecID`的数量超过`109`个，就需要额外的sector来储存多出来的`SecID`，我们把这种sector叫做MsatSector。

`header._sectDifStart`就是第`1`个MsatSector的`SecID`（如果MSAT不需要额外的sector，它的值为`-2`）。每个MsatSector保存的最后一个SecID指向了下一个MSAT需要的用到的MsatSector，如果接下来没有MsatSector的话这个值是`-2`。因此每个MsatSector保存了127个SatSector的`SecID`。需要通过一个预处理把MSAT提取出来，然后根据MSAT中的`SecID`依次把SAT提取出来。


### 目录流和目录项：

目录流是一种特殊的流，它是一个目录项（directory entry）的数组。`header._sectDirStart`指向了目录流所占用的第一个sector的SecID。每个目录项占用128字节。

目录项按照他们在目录流中的顺序从0开始依次有个编号（DirID），根据DirID可以计算出这个DirID所表示的`directory_entry`相对于目录流开始位置的相对位移`dir_entry_pos(DirID) = DirID*128`。通过`DirectoryEntry`的连接，流之间的组织结构和文件系统的结构相似。

![alt text](http://tsfn.ws/wp-content/uploads/2014/02/03-300x75.png "各种流之间的组织结构和文件系统相似")

由于目录之间的连接结构过于复杂，而且在提取doc文本的过程中用不到，所以我们只需要知道我们可以通过流的名字在目流项的列表中找流的入口。

根据`DirID`提取directory的方法就是将目录流中`dir_entry_pos(DirID)`开始的`128`字节按如下结构解释：

    typedef enum tagSTGTY {
      STGTY_INVALID		 = 0,
      STGTY_STORAGE		 = 1,
      STGTY_STREAM		 = 2,
      STGTY_LOCKBYTES	 = 3,
      STGTY_PROPERTY 	 = 4,
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
    struct StructuredStorageDirectoryEntry {
      uchar _ab[32*sizeof(WCHAR)]; //[0x00]这个directory_entry所表示的流的名字，按UTF-16保存
      ushort _cb; //[0x40] _ab保存的名字的长度，按有多少字算，不是按字节算
      uchar _mse; //[0x42] 对象的类型，值取STGTY中的一个
      uchar _bflags; //[0x43] 值是DECOLOR中的一个
      uint _sidLeftSib; //[0x44] 当前项的左兄弟的DirID
      uint _sidRigthSib; //[0x48] 当前项的右兄弟的DirID
      uint _sidChild; //[0x4C] 当前元素的所有节点中的根节点
      uchar _clsId[16]; //[0x50] 当前storage的CLSID（如果_mse=STGTY_STORAGE）
      uint _dwUserFlags; //[0x60] 当前storage的用户标记（如果_mse=STGTY_STORAGE）
      TIME_T _time[2]; //[0x64] Create/Modify时间戳
      uint _sectStart; //*[0x74] 流的起始sector的SecID
      uint _ulSize; //*[0x78] 流的大小，单位是字节
      ushort _dptPropType; //[0x7C] 一定都是0
    };

直接提取出所有的目录项是很简单的，但是如果需要对doc文件进行进一步的解析，比如提取图片、文本格式、表格等，就要用到目录之间的关系了。


## 短流：

当一个流的长度小于`header._ulMiniSectorCutoff`，这个流叫做短流（Short-Stream）。

按短流的第一个SecID提取短流的方法和提取普通流的方法大致相同。但是短流不直接用sector来保存数据，而是需要先提取出Short-Stream Container Stream(SSCS)和Short-Sector Allocation Table(SSAT)。

SSAT是一个保存SecID的数组，保存了所有短流的SecID链，和SAT中的SecID链的形式一样。它由`header._sectMiniFatStart`和SAT得到，取得的方法和取得一般流的方法一样。

SSCS按每`(1<<header._uMiniSectorShift)`字节划分成若干short-sector，按短流的SecID寻找某个short-sector在SSCS中的位置`short_sec_pos(SecID)=SecID*(1<<header._uMiniSectorShift)`。SSCS由目录流（directory stream）中的第一个目录项，即`DirID=0`的目录项的`_sectStart`和SAT得到，取得的方法和取得一般流的方法一样。


# 根据WordDocument Stream和Table Stream提取文本：

文本被分成几段（几个pieces）保存在WordDocument流中。由于pieces在流中的位置不是固定的，它们的位置需要通过piece表（piece table）查找得到，而piece表保存在一个叫clx结构中，而clx的位置又需要通过WordDocument Stream中的特定字段得到。

WordDocument Stream的开头是一个FIB结构，其中相关的字段如下：

    struct FIB {
        struct FibBase base;
        uchar arr1[122];
        struct FibRgFcLcb97 fibRgFcLcb97;
    };

    struct FibBase {
        ushort wIdent;	// 用来标记的字段，一定是0xA5EC
        ushort nFib;	// 指定文档版本号，被FibRgCswNew.nFibNew取代
        ushort unused;	// 未定义
        ushort lid;	// 标记创建本文档的应用程序支持的语言
        ushort pnNext;	// 如果文档中包括AutoText项目
                        // pnNext指明了它在WordDocument流中的位置
        ushort fDot: 1; // 标志文件是否是文档模板（document template）
        ushort fGlsy: 1; // 标志文件是否只包括AutoText项目
        ushort fComplex: 1; // 标志文件的最后一次操作是否是
                          // 增量保存（incremental save）
        ushort fHasPic: 1; // 如果是0，文件中应该没有图片 uchar envr;
        ushort cQuickSaves: 4; // 如果nFib小于0x00D9，
                              // cQuickSaves是文件连续增量保存的次数；
                              // 如果nFib大于等于0x00D9，cQuickSaves一定是0xF
        ushort fEncrypted: 1; // 标志文件是否被加密或被扰乱（obfuscated）
        ushort fWhichTblStm: 1; // 标志FIB引用的是1Table(fWhichTblStm=1)
                              // 还是0Table(fWhichTblStm=0)。
        ushort fReadOnlyRecommended: 1; // 标志文件是否被作者设置为建议只读
        ushort fWriteReservation: 1; // 标记文件是否有写入密码
        ushort fExtChar: 1; // 一定是1
        ushort fLoadOverride: 1; // 标志是否要用应用程序使用的语言和格式
        ushort fFarEast: 1; // 标志应用程序使用的语言是否是东亚语言
        ushort fObfuscated: 1; // 如果fEncrypted=1，
                             // fObfuscated标志了文件是否用XOR扰乱过
        ushort nFibBack;	// 值只可能是0x00BF或0x00C1
        uint IKey;	// fEncrypted=1且fObfuscation=1时它个验证值，
                        // fEncrypted=1且fObfuscation=0时它是
                        // EncryptionHeader的大小，否则它一定是0。
        uchar envr;	// 一定是0
        uchar fMac: 1; // 一定是0
        uchar fEmptySpecial: 1; // 应该是0
        uchar fLoadOverridePage: 1; // 标志是否用应用程序默认的页面大小、方向和留空
        uchar reserved1: 1;
        uchar reverved2: 1;
        uchar fSpare0: 3; // 未定义
        ushort reserved3;
        ushort reserved4;
        uint reverved5;
        uint reverved6;
    };

    struct FibRgFcLcb97 {
        uint fcStshfOrig;
        uint lcbStshfOrig;
        uint fcStshf;
        uint lcbStshf;
        uint fcPlcffndRef;
        uint lcbPlcffndRef;
        uint fcPlcffndTxt;
        uint lcbPlcffndTxt;
        uint fcPlcfandRef;
        uint lcbPlcfandRef;
        uint fcPlcfandTxt;
        uint lcbPlcfandTxt;
        uint fcPlcfSed;
        uint lcbPlcfSed;
        uint fcPlcPad;
        uint lcbPlcPad;
        uint fcPlcfPhe;
        uint lcbPlcfPhe;
        uint fcSttbfGlsy;
        uint lcbSttbfGlsy;
        uint fcPlcfGlsy;
        uint lcbPlcfGlsy;
        uint fcPlcfHdd;
        uint lcbPlcfHdd;
        uint fcPlcfBteChpx;
        uint lcbPlcfBteChpx;
        uint fcPlcfBtePapx;
        uint lcbPlcfBtePapx;
        uint fcPlcfSea;
        uint lcbPlcfSea;
        uint fcSttbfFfn;
        uint lcbSttbfFfn;
        uint fcPlcfFldMom;
        uint lcbPlcfFldMom;
        uint fcPlcfFldHdr;
        uint lcbPlcfFldHdr;
        uint fcPlcfFldFtn;
        uint lcbPlcfFldFtn;
        uint fcPlcfFldAtn;
        uint lcbPlcfFldAtn;
        uint fcPlcfFldMcr;
        uint lcbPlcfFldMcr;
        uint fcSttbfBkmk;
        uint lcbSttbfBkmk;
        uint fcPlcfBkf;
        uint lcbPlcfBkf;
        uint fcPlcfBkl;
        uint lcbPlcfBkl;
        uint fcCmds;
        uint lcbCmds;
        uint fcUnused1;
        uint lcbUnused1;
        uint fcSttbfMcr;
        uint lcbSttbfMcr;
        uint fcPrDrvr;
        uint lcbPrDrvr;
        uint fcPrEnvPort;
        uint lcbPrEnvPort;
        uint fcPrEnvLand;
        uint lcbPrEnvLand;
        uint fcWss;
        uint lcbWss;
        uint fcDop;
        uint lcbDop;
        uint fcSttbfAssoc;
        uint lcbSttbfAssoc;
        uint fcClx; // 提取文本时用到
        uint lcbClx;
        uint fcPlcfPgdFtn;
        uint lcbPlcfPgdFtn;
        uint fcAutosaveSource;
        uint lcbAutosaveSource;
        uint fcGrpXstAtnOwners;
        uint lcbGrpXstAtnOwners;
        uint fcSttbfAtnBkmk;
        uint lcbSttbfAtnBkmk;
        uint fcUnused2;
        uint lcbUnused2;
        uint fcUnused3;
        uint lcbUnused3;
        uint fcPlcSpaMom;
        uint lcbPlcSpaMom;
        uint fcPlcSpaHdr;
        uint lcbPlcSpaHdr;
        uint fcPlcfAtnBkf;
        uint lcbPlcfAtnBkf;
        uint fcPlcfAtnBkl;
        uint lcbPlcfAtnBkl;
        uint fcPms;
        uint lcbPms;
        uint fcFormFldSttbs;
        uint lcbFormFldSttbs;
        uint fcPlcfendRef;
        uint lcbPlcfendRef;
        uint fcPlcfendTxt;
        uint lcbPlcfendTxt;
        uint fcPlcfFldEdn;
        uint lcbPlcfFldEdn;
        uint fcUnused4;
        uint lcbUnused4;
        uint fcDggInfo;
        uint lcbDggInfo;
        uint fcSttbfRMark;
        uint lcbSttbfRMark;
        uint fcSttbfCaption;
        uint lcbSttbfCaption;
        uint fcSttbfAutoCaption;
        uint lcbSttbfAutoCaption;
        uint fcPlcfWkb;
        uint lcbPlcfWkb;
        uint fcPlcfSpl;
        uint lcbPlcfSpl;
        uint fcPlcftxbxTxt;
        uint lcbPlcftxbxTxt;
        uint fcPlcfFldTxbx;
        uint lcbPlcfFldTxbx;
        uint fcPlcfHdrtxbxTxt;
        uint lcbPlcfHdrtxbxTxt;
        uint fcPlcffldHdrTxbx;
        uint lcbPlcffldHdrTxbx;
        uint fcStwUser;
        uint lcbStwUser;
        uint fcSttbTtmbd;
        uint lcbSttbTtmbd;
        uint fcCookieData;
        uint lcbCookieData;
        uint fcPgdMotherOldOld;
        uint lcbPgdMotherOldOld;
        uint fcBkdMotherOldOld;
        uint lcbBkdMotherOldOld;
        uint fcPgdFtnOldOld;
        uint lcbPgdFtnOldOld;
        uint fcBkdFtnOldOld;
        uint lcbBkdFtnOldOld;
        uint fcPgdEdnOldOld;
        uint lcbPgdEdnOldOld;
        uint fcBkdEdnOldOld;
        uint lcbBkdEdnOldOld;
        uint fcSttbfIntlFld;
        uint lcbSttbfIntlFld;
        uint fcRouteSlip;
        uint lcbRouteSlip;
        uint fcSttbSavedBy;
        uint lcbSttbSavedBy;
        uint fcSttbFnm;
        uint lcbSttbFnm;
        uint fcPlfLst;
        uint lcbPlfLst;
        uint fcPlfLfo;
        uint lcbPlfLfo;
        uint fcPlcfTxbxBkd;
        uint lcbPlcfTxbxBkd;
        uint fcPlcfTxbxHdrBkd;
        uint lcbPlcfTxbxHdrBkd;
        uint fcDocUndoWord9;
        uint lcbDocUndoWord9;
        uint fcRgbUse;
        uint lcbRgbUse;
        uint fcUsp;
        uint lcbUsp;
        uint fcUskf;
        uint lcbUskf;
        uint fcPlcupcRgbUse;
        uint lcbPlcupcRgbUse;
        uint fcPlcupcUsp;
        uint lcbPlcupcUsp;
        uint fcSttbGlsyStyle;
        uint lcbSttbGlsyStyle;
        uint fcPlgosl;
        uint lcbPlgosl;
        uint fcPlcocx;
        uint lcbPlcocx;
        uint fcPlcfBteLvc;
        uint lcbPlcfBteLvc;
        uint dwLowDateTime;
        uint dwHighDateTime;
        uint fcPlcfLvcPre10;
        uint lcbPlcfLvcPre10;
        uint fcPlcfAsumy;
        uint lcbPlcfAsumy;
        uint fcPlcfGram;
        uint lcbPlcfGram;
        uint fcSttbListNames;
        uint lcbSttbListNames;
        uint fcSttbfUssr;
        uint lcbSttbfUssr;
    };

### 根据WordDocument Stream中的字段寻找Table Stream

如果FIB.base.fWhichTblStm=1，则Table Stream的名字是`"1Table"`，否则它的名字是`"0Table"`。

### 根据WordDocument Stream中的字段寻找Table Stream中的clx结构体

Table Stream的FIB.fibRgFcLcb97.fcClx偏移位置开始的FIB.fibRgFcLcb97.lcbClx字节是clx结构体。

### 在clx内部找到Piece Table

clx中的结构如下，其中的RgPrc数组是不需要的，但是它有几个元素是不知道的、它的每个元素的大小是多少也不知道。

为了跳过这个数组，我们需要先读取开头的一个字节，根据这个字节判断当前的结构是struct Prc还是struct Pcdt。

如果是Prc，则读取它的大小，然后跳过它；如果是Pcdt，则读取它的大小，然后得到这个结构。

    struct Prc {
        uchar clxt;
        ushort cbGrpprl;
        uchar grpprl[unknown];
    };

    struct Pcdt {
        uchar clxt;
        uint lcb;
        struct PlcPcd piece_table; // 这是我们需要找的结构，它的大小未知
    };

    struct Clx { // 我们现在有这个结构的首地址
        struct Prc RgPrc[unknown];
        struct Pcdt pcdt;
    };

这个过程需要算法：

    // 假设我们有一个uchar *clx，需要得到的Piece Table的大小保存在*plcb
    uchar *get_piece_table(uchar *clx, uint *plcb) {
        int pos = 0;
        for (;;) {
            uchar clxt = clx[pos];
            pos += 1; // 跳过clxt
            if (clxt == 1) {
                ushort cbGrpprl = *(ushort *)(clx + pos); // grpprl的大小
                pos += 2; // 跳过cbGrpprl
                pos += cbGrpprl; // 跳过grpprl
            } else if (clxt == 2) {
                *plcb = *(uint *)(clx + pos);  // Piece Table的大小
                pos += 4;
                return clx + pos;
            } else {
                return NULL; // 非法值
            }
        }
        return NULL;
    }


### 根据Piece Table从WordDocument Stream中提取文本

Piece Table的结构如下，其中`n`是一个未知量，需要通过Pcdt.lcb（就是上面的过程中得到的`*plcb`）计算得到，`n=(Pcdt.lcb-4)/12`：

    struct PlcPcd { // Piece Table的结构
        uint CP[n + 1];
        struct Pcd pcd[n];
    };

    struct Pcd {
        ushort reversed;
        struct FcCompressed fcCompressed; // 文本块在WordDocument Stream中的位置
        ushort prm; // 用来描述直接段格式和直接字格式
    }

Piece Table包含两个数组：

  * 第一个数组包含`n+1`个字符的逻辑位置（`n`是片段piece的数量），逻辑位置是指字符在文本中是第几个，比如第一个元素是第一个片段在整个文本中的起始位置，第二个元素是第一个片段在文本中的结束位置、同时也是第二个片段在文本中的开始位置。
  * 第二个数组包含了n个片段描述结构（struct FcCompressed），表示相应的逻辑片段在WordDocument Stream中的起始位置。

FcCompressed结构如下：

    struct FcCompressed {
        uint fc: 30;
        uint fCompressed: 1; // 表示当前文本块是否被压缩过
        uint r1: 1; // 一定是0
    };

* 如果fCompressed=0，文本块是一个在WordDocument Stream中从fc开始的16-bit的Unicode的字符数组（UTF-16LE）
* 如果fCompressed=1，文本块是一个在WordDocument Stream中从fc/2开始的8-bit的Unicode的字符数组（ASCII）

从Piece Table提取文本的过程：

    void retrive_text(uchar *WordDocumentStream, uchar *piece_table, uint lcb) {
        FILE *output = fopen("output.txt", "wb");
        uint n = (lcb - 4) / 12;
        for (uint i = 0; i < n; ++i) {
            uint cp_start = *(uint *)(piece_table + i * 4);
            uint cp_end = *(uint *)(piece_table + i * 4 + 4);
            uint cb = cp_end – cp_start;
            if (((fcCompressed >> 30) & 1) == 0) {
                uint fc = fcCompressed;
                cb *= 2;
                for (uint j = 0; j < cb; ++j) {
                    fputc(WordDocumentStream[fc + j], output);
                }
            } else {
                fc = (fcCompressed & ~(1U << 30)) / 2;
                for (uint j = 0; j < cb; ++j) {
                    // 把一个8-bit的字符扩展成16-bit的字符
                    fputc(WordDocumentStream[fc + j], output);
                    fputc(0, output);
                }
            }
        }
        fclose(output);
    }

# 提取图片

## 提取OfficeArtContent

根据FIB.FibRgFcLcb97.fcDggInfo和FIB.FibRgFcLcb97.lcbDggInfo从Table Stream中得到OfficeArtContent结构。

    uchar *getOfficeArtContent(uchar *word_document_stream, uchar *table_stream) {
        FIB *fib = (FIB *)word_document_stream;
        uchar *OfficeArtContent = table_stream + fib->FibRgFcLcb97.fcDggInfo;
        return OfficeArtContent;
    }

### 解析OfficeArtContent结构，找到其中的OfficeArtBlip结构

    struct OfficeArtContent {
        struct OfficeArtRecordHeader rh;
        struct OfficeArtRecordHeader rh2;
        uchar arr[unknown];
        struct OfficeArtBStoreContainer blipStore;
    };

    struct OfficeArtRecordHeader {
        ushort recVer: 4;
        ushort recInstance: 12;
        ushort recType; // 0xF000~0xFFFF
        uint recLen; // 所在结构体的长度，不包括本结构体
    };

    struct OfficeArtBStoreContainer {
        struct OfficeArtRecordHeader rh;
        struct OfficeArtBStoreContainerFileBlock rgfb[unkown];
    };

    struct OfficeArtBStoreContainerFileBlock {
        OfficeArtRecordHeader rh;
    };

跳过头部没有用到的部分，得到blipStore和rgfb：

    struct OfficeArtBStoreContainer *blipStore = (struct OfficeArtBStoreContainer *)((uchar *)OfficeArtContent + 16 + OfficeArtContent->rh2.recLen);
    struct OfficeArtBStoreContainerFileBlock *rgfb = (struct OfficeArtBStoreContainerFileBlock *)((uchar *)blipStore + 8);

blipStore的大小等于`8+blipStore->rh.recLen`，可以用这个值来判断rgfb中的元素是否取完。

rgfb是个OfficeArtBStoreContainerFileBlock的数组，其中的每个元素指向了一个图片。

对于rgfb[]中的每个元素rgfb[i]，rgfb[i].rh.recType=0xF007时rgfb[i]是一个OfficeArtFBSE结构，这时存储图片的OfficeArtBlip结构在WordDocument Stream中的foDelay位置；否则rgfb[i]是一个OfficeArtBlip结构，这时图片内嵌在当前结构中。

    struct OfficeArtFBSE {
        struct OfficeArtRecordHeader rh;
        uchar arr[20];
        uint size; // BLIP占用的字节数
        uint cRef;
        uint foDelay;
        uchar unused1;
        uchar cbName;
        ushort unused2;
        wchar_t nameData[unknown]; // BLIP的名字
        OfficeArtBlip embeddedBlip;
    };

    struct OfficeArtBlip {
    };

示例过程：

    while (rgfb < (uchar *)blipStore + blipStore->rh.recLen) {
        ushort recType = rgfb->rh.recType;
        ushort recSize = rgfb->rh.recSize;
        if (recType == 0xF007) {
            struct OfficeArtFBSE *officeArtFBSE = rgfb;
            uint size = officeArtFBSE->size;
            uint foDelay = officeArtFBSE->foDelay;
            storeBlipBlock(word_document_stream + foDelay, "img_name", size);
        } else {
            storeBlipBlock(rgfb, img_name, recSize);
        }
    }

### 解析OfficeArtBlip

这里的OfficeArtBlip结构可能是从WordDocument Stream中取得的，也可能是包含在OfficeArtContent的。

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

示例过程：

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

## 内嵌图片

按Ctrl-V拷贝进Word文件的图片和选择从文件“插入”的图片存放的位置不一样。提取内嵌图片需要用到提取字符格式时得到的信息。

如果某个字符的值是U+0001，且它的sprm=0x6A03，并且它没有(sprm=0x0806,operand=1)的属性，则这个字符的参数表示Data Stream中的PICFAndOfficeArtData结构的偏移，这个结构保存了图片。

先提取Data Stream，再根据参数找到PICF结构：

    struct PICFAndOfficeArtData *PICF = (struct PICFAndOfficeArtData *)((uchar *)data_stream + operand);

PICF的结构：

    struct PICFAndOfficeArtData {
        uint lcb; // PICF的大小
        uchar arr1[2];
        ushort mm; // 当mm=0x66时，cchPicName字段存在，否则cchPicName不存在
        uchar arr2[60];
    };

    struct PICFAndOfficeArtData2 {
        uchar cchPicName; // 这个字段可能不存在，如果存在的话需要被跳过
        uchar arr3[unknown]; // 这个字段可能不存在
    };

    struct PICFAndOfficeArtData3 {
        struct OfficeArtRecordHeader rh;
        uchar arr4[unknown]; // 大小是rh.recLen，需要被跳过
        struct OfficeArtBStoreContainerFileBlock rgfb[];
    };

找到其中的rgfb数组，然后调用相同的函数处理OfficeArtBlip结构：

    uint lcbPICF = PICF->lcb;
    ushort mm = PICF->mm;
    PICFAndOfficeArtData3 *PICF3;
    if (mm == 0x0066) {
        PICFAndOfficeArtData2 *PICF2 = (PICFAndOfficeArtData3 *)((uchar *)PICF + 68);
        PICF3 = (PICFAndOfficeArtData3 *)((uchar *)PICF + 69 + PICF2->cchPicName);
    } else {
        PICF3 = (PICFAndOfficeArtData3 *)((uchar *)PICF + 68);
    }
    uchar *rgfb = (uchar *)PICF3 + 8 + PICF3->rh.recLen;
    while (rgfb < PICF + lcbPICF) {
        uint recSize = ((OfficeArtRecordHeader *)rgfb)->recSize;
        uchar cbName = *(rgfb + 41);
        storeBlipBlock(rgfb + 44, "img_name", recSize - cbName - 36);
        rgfb += 8 + recSize;
    }


# 从doc文件中提取格式信息

MS-Word中的格式（format）有文字格式、段落格式、图片格式、表单格式等。加粗/斜体和字体大小等属于文字格式，居中/右对齐等属于段落格式。

### 根据WordDocument Stream中的字段找到Table Stream中的结构

根据WordDocument Stream中的FIB.fibRgFcLcb97.fcPlcfBteChpx字段和FIB.fibRgFcLcb97.fcPlcfBtePapx字段，分别从Table Stream找到PlcBteChpx结构和PlcBtePapx结构。文字格式用到PlcfBteChpx结构，段落格式用到PlcBtePapx结构。

PlcBteChpx和PlcBtePapx都在Table Stream中，需要根据它们找到在WordDocument Stream中的多个ChpxFkp和PapxFkp结构。

PlcBteChpx和PlcBtePapx中的相关信息：

    struct PlcBteChpx {
        uint FC[unknown]; // 一个uint的数组，表示取到哪些文本，没有用到
        uint PnBteChpx[unknown]; // ChpxFkp在WordDocument Stream中的偏移
    };

    struct PlcBtePapx {
        uint FC[unknown]; // 一个uint的数组，表示取到哪些文本，没有用到
        uint PnBtePapx[unknown]; // PapxFkp在WordDocument Stream中的偏移
    };

根据微软的文档，需要根据FIB.fibRgFcLcb97.lcbPlcfBteChpx计算PlcBteChpx中数组的元素个数`n = (FIB.fibRgFcLcb97.lcbPlcfBteChpx - 4) / 8`。然后从Table Stream得到每个PnFkpChpx中的元素和PnFkpPapx中的元素，PnFkpChpx的第`i`个元素表示在WordDocument Stream中偏移量为`PnFkpChpx[i]*512`的位置有一个相应的ChpxFkp结构：

    uint i, n;
    uchar *PlcBteChpx = table_stream + fcPlcfBteChpx;
    uchar *PlcBtePapx = table_stream + fcPlcfBtePapx;

    n = (FIB.fibRgFcLcb97.lcbPlcfBteChpx - 4) / 8;
    for (i = 0; i < n; i += 1) {
        uint PnFkpChpx = *(uint *)(PlcBteChpx + 4 * (n + 1 + i));
        uchar *ChpxFkp = wds + PnBteChpx * 512;
        // 得到ChpxFkp，对它进行处理
    }

    n = (FIB.fibRgFcLcb97.lcbPlcfBtePapx - 4) / 8;
    for (i = 0; i < n; i += 1) {
        uint PnFkpPapx = *(uint *)(PlcBtePapx + 4 * (n + 1 + i));
        uchar *PapxFkp = wds + PnBtePapx * 512;
        // 得到PapxFkp，对它进行处理
    }


### ChpxFkp

ChpxFkp的大小固定是512字节，它的最后一个字节表示一个无符号整型数`crun`，`crun`的取值范围必须在0x01和0x65之间。

    struct ChpxFkp {
        uint rgfc[unknown]; // crun + 1个元素的uint数组，表示WordDocument Stream中的偏移量
        uchar rgbx[unknown]; // rgbx[i]*2是Chpx在这个ChpxFkp中的位移
        struct Prl grpprl[unknown];
        uchar crun; // 0x01 <= crun <= 0x65
    };

其中rgfc有`crun+1`个元素，rgb有`crun`个元素。

相邻的两个rgfc指定了一段文本，对应的rgbx指定了一个Chpx，这个Chpx中的grpprl部分描述了这段文本的格式。如果rgb等于0，则没有Chpx描述这段文字。

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

ChpxFkp的大小固定是512字节，它的最后一个字节表示一个无符号整型数`cpara`，`cpara`的取值范围必须在0x01和0x1D之间。

    struct BxPap {
        uchar bOffset;
        uchar reserved[12];
    };

    struct PapxInFkp {
        uchar cb;
        uchar cbt[unknown];
        struct Prl grpprl[unknown];
    };

    struct PapxFkp {
        uint rgfc[unknown]; // cpara + 1个元素的uint数组，表示WordDocument Stream中的偏移量
        struct Bxpap bxpap[unknown];
        struct PapxInFkp papxInFkp[unknown];
        uchar cpara; // 0x01 <= cpara <= 0x1D
    };

rgfc有`cpara+1`个元素，rgb有`cpara`个元素。

相邻的两个rgfc指定了一段文本，对应的rgbx指定了一个PapxInFkp，其中的grpprl描述了这段文本的段落格式。

这里的grpprl和在ChpxFkp中的grpprl结构一样，处理方式也类似，但是它需要特殊处理`cbt`字段。

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

    struct Sprm {
        ushort ispmd: 9;
        ushort fSpec: 1;
        ushort sgc: 3;
            // 格式类型
            // sgc = 1 时 sprm是一个段落格式（paragraph property）
            // sgc = 2 时 sprm是一个文字格式（character property）
            // sgc = 3 时 sprm是一个图片格式（picture property）
            // sgc = 4 时 sprm是一个section property
            // sgc = 5 时 sprm是一个table property
        ushort spra: 3;
            // 参数operand的大小，
            // spra = 0 时 operand占1字节
            // spra = 1 时 operand占1字节
            // spra = 2 时 operand占2字节
            // spra = 3 时 operand占4字节
            // spra = 4 时 operand占2字节
            // spra = 5 时 operand占2字节
            // spra = 6 时 operand是变长的
            // spra = 7 时 operand占3字节
    };

    struct Prl {
        struct Sprm sprm; // 指定格式的类型
        uchar operand[unknown]; // 格式的参数
    };

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
