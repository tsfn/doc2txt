# doc文件存储格式以及从中提取文本的方法

[维基百科](http://en.wikipedia.org/wiki/DOC_%28computing%29)
[MSDN文档](http://msdn.microsoft.com/en-us/library/dd904907)
[我的代码](http://pan.baidu.com/s/1i3oOSZj)

doc文件是一种二进制复合文件（Binary Compound File），其中存储了若干个流（stream），一个流就是一串连续的字节，一个流可能是表示文件内部结构的数据、可能是用户数据、可能是一个二进制文件、或文本文件等。

每个流都有一个名字，用一个字符数组表示，doc文件的文本保存在其中一个名字叫*Document Stream*的流当中。为了通过名字取出某个流，我们需要分析二进制复合文件的内部结构。

`ole.h`中的`Storage`类通过`bool init(FILE *file)`初始化之后通过它的`bool read_stream(const char *name, vector<uint_8> &stream)`方法读出某个特定名字的流。


## doc文件的基本结构：

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
      ushort _uByteOrder; // [0x1C]0xFFFE表示大端存储，0xFEFF表示小端存储，一般是0xFEFF
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


## SecID的取值：

  * >= 0 表示某个sector的ID
  * -1 （Free SecID），空sector，表示这个sector不属于任何流
  * -2 （End Of Chain SecID），表示结尾
  * -3 （SAT SecID），表示这个sector被用于存放sector分配表（sector allocation table）
  * -4 （MSAT SecID），表示这个sector被用于存放MSAT(master sector allocation table)


## SAT的用法：

SAT是一个由MSAT得到的特殊的数组，其中当前位置（数组下标）表示当前sector的`SecID`，而这个数组在这个位置的值指的是下一个sector的`SecID`。

![alt text](http://tsfn.ws/wp-content/uploads/2014/02/02.jpg "SAT的用法")

比如说我们已经知道了目录流（Directory Stream，用来存放目录的流）的占用的第一个sector的`SecID`是`0`，那么为了得到这个流，我们先从`sec_pos(0)`处取出`SecID`为`0`的sector，然后根据`SAT[0]=2`得知目录流的下一个sector的`SecID`是`2`、并从`sec_pos(2)`处取出`SecID`为`2`的sector，然后根据`SAT[2]=3`得知目录流的下一个sector的`SecID`是`3`、并从`sec_pos(3)`处取出`SecID`为`3`的sector，然后根据`SAT[3]=-2`得知目录流没有下一个sector了。像这样的`SecID`的序列被称为`SecID`链（SecID chain）。


## 如何得到MSAT和SAT：

为了得到SAT，我们需要先得到MSAT（Master Sector Allocation Table），MSAT是一个存放SecID的数组，这些SecID表示SAT用到了哪些sector。我们把SAT用到的sector叫做SatSector。

MSAT的前109个SecID存放在`header._sectFat`中，如果MSAT包含的`SecID`的数量超过`109`个，就需要额外的sector来储存多出来的`SecID`，我们把这种sector叫做MsatSector。

`header._sectDifStart`就是第`1`个MsatSector的`SecID`（如果MSAT不需要额外的sector，它的值为`-2`）。每个MsatSector保存的最后一个SecID指向了下一个MSAT需要的用到的MsatSector，如果接下来没有MsatSector的话这个值是`-2`。因此每个MsatSector保存了127个SatSector的`SecID`。需要通过一个预处理把MSAT提取出来，然后根据MSAT中的`SecID`依次把SAT提取出来。


## 目录流和目录项：

目录流是一种特殊的流，它是一个目录项（directory entry）的数组。`header._sectDirStart`指向了目录流所占用的第一个sector的SecID。每个目录项占用128字节。

目录项按照他们在目录流中的顺序从0开始依次有个编号（DirID），根据DirID可以计算出这个DirID所表示的`directory_entry`相对于目录流开始位置的相对位移`dir_entry_pos(DirID) = DirID*128`。通过`DirectoryEntry`的连接，流之间的组织结构和文件系统的结构相似。

![alt text](http://tsfn.ws/wp-content/uploads/2014/02/03-300x75.png "各种流之间的组织结构和文件系统相似")

由于目录之间的连接结构过于复杂，而且在提取doc文本的过程中用不到，所以我们只需要知道我们可以通过流的名字在目流项的列表中找流的入口。

根据`DirID`提取directory的方法就是将目录流中`dir_entry_pos(DirID)`开始的`128`字节按如下结构解释：

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

直接提取出所有的目录项是很简单的，但是如果需要对doc文件进行进一步的解析，比如提取图片、文本格式、表格等，就要用到目录之间的关系了。


## 短流：

当一个流的长度小于`header._ulMiniSectorCutoff`，这个流叫做短流（Short-Stream）。

按短流的第一个SecID提取短流的方法和提取普通流的方法大致相同。但是短流不直接用sector来保存数据，而是需要先提取出Short-Stream Container Stream(SSCS)和Short-Sector Allocation Table(SSAT)，然后通过SSAT和SSCS得到所需要的流。

SSAT是一个保存SecID的数组，保存了所有短流的SecID链，和SAT中的SecID链的形式一样。它由`header._sectMiniFatStart`和SAT得到，取得的方法和取得一般流的方法一样。

SSCS按每`(1<<header._uMiniSectorShift)`字节划分成若干short-sector，按短流的SecID寻找某个short-sector在SSCS中的位置`short_sec_pos(SecID)=SecID*(1<<header._uMiniSectorShift)`。SSCS由目录流（directory stream）中的第一个目录项，即`DirID=0`的目录项的`_sectStart`和SAT得到，取得的方法和取得一般流的方法一样。


## 根据WordDocument流提取文本：

文本被分成几段（几个pieces）保存在WordDocument流中。由于pieces在流中的位置不是固定的，它们的位置需要通过piece表（piece table）查找得到，而piece表保存在一个叫`clx`字节数组中，而`clx`的位置需要根据WordDocument流中的特定字段得到。

WordDocument流的开头32字节是`FibBase`，结构如下：

    struct FibBase {
      ushort wIdent;		// [0x00] 用来标记的字段，一定是0xA5EC
      ushort nFib;		// [0x02] 指定文档版本号，被FibRgCswNew.nFibNew取代
      ushort unused;		// [0x04] 未定义
      ushort lid;		// [0x06] 标记创建本文档的应用程序支持的语言
      ushort pnNext;	// [0x08] 如果文档中包括AutoText项目，pnNext指明了它在WordDocument流中的位置
      ushort flag1;	// [0x0A] 见下
      ushort nFibBack;	// [0x0C] 值只可能是0x00BF或0x00C1
      uint IKey;		// [0x0E] fEncrypted=1且fObfuscation=1时它个验证值，fEncrypted=1且fObfuscation=0时它是EncryptionHeader的大小，否则它一定是0。
      uchar envr;		// [0x12] 一定是0
      uchar flag2;		// [0x13] 见下
      ushort reserved3;	// [0x14] 未定义
      ushort reserved4;	// [0x16] 未定义
      uint reverved5;	// [0x18] 未定义
      uint reverved6;	// [0x1C] 未定义
    };

* flag1中的16个比特位从低位到高位分别是：

  * A – fDot (1 bit) 标志文件是否是文档模板（document template）
  * B – fGlsy (1 bit) 标志文件是否只包括AutoText项目
  * C – fComplex (1 bit) 标志文件的最后一次操作是否是增量保存（incremental save）
  * D – fHasPic (1 bit) 如果是0，文件中应该没有图片
  * E – cQuickSaves (4 bits) 如果nFib小于0x00D9，cQuickSaves是文件连续增量保存的次数；如果nFib大于等于0x00D9，cQuickSaves一定是0xF
  * F – fEncrypted (1 bit) 标志文件是否被加密或被扰乱（obfuscated）
  * G – fWhichTblStm (1 bit) 标志FIB引用的是1Table(fWhichTblStm=1)还是0Table(fWhichTblStm=0)。但是实际中通过wds[0x0B]&0x02才能确定是0Table还是1Table。
  * H – fReadOnlyRecommended (1 bit) 标志文件是否被作者设置为建议只读
  * I – fWriteReservation (1 bit) 标记文件是否有写入密码
  * J – fExtChar (1 bit) 一定是1
  * K – fLoadOverride (1 bit) 标志是否要用应用程序使用的语言和格式
  * L – fFarEast (1 bit) 标志应用程序使用的语言是否是东亚语言
  * M – fObfuscated (1 bit) 如果fEncrypted=1，fObfuscated标志了文件是否用XOR扰乱过

* flag2中的8个比特位从低位到高位分别是：

  * N – fMac (1 bit) 一定是0
  * O – fEmptySpecial (1 bit) 应该是0
  * P – fLoadOverridePage (1 bit) 标志是否用应用程序默认的页面大小、方向和留空
  * Q – reserved1 (1 bit) 未定义
  * R – reverved2 (2 bit) 未定义
  * S – fSpare0 (3bits) 未定义


### 根据WordDocument流得到clx字节数组

WordDocument中相关的字段如下：

    WordDocument Stream {
     FIB {
       32bytes FibBase; {
         2bytes wIdent;
         2bytes nFib;
         ... // 其他字段
         1bit G-fWhichTblStm; // 标志所用的Table Stream是0Table还是1Table
         ... // 其他字段
       }
       2bytes csw;
       28bytes fibRgW;
       2bytes cslw;
       88bytes fibRgLw;
       2bytes cbRgFcLcb; //指明fibRgFcLcb的版本
       Variable fibRgFcLcb; // (fibRgFcLcbBlob，FIB中第一个变长的结构) {
         744bytes fibRgFcLcb97; {
           ... // 其他字段
           4bytes fcClx; // WDS[0x01A2] Clx在Table Stream中的偏移位置
           4bytes lcbClx; // WDS[0x01A6] Clx的长度
                   // fcClx和lcbClx在WordDocument Stream中的相对位置是固定的
           ... // 其他字段
         }
         ... // 其他字段
       }
       ... // 其他字段
     }
     ... // 其他字段
    }

根据上表，我们可以用如下过程得到clx数组：

    table_stream = readStream((get_ushort(WordDocumentStream, 0x000A)&0x0200)==0x0200 ? “1Table” : “0Table”);
    clxOffset = get_uint(WordDocumentStream, 0x01A2);
    clxLength = get_uint(WordDocumentStream, 0x01A6);
    clx = table_stream[clxOffset ... clxOffset+clxLength];

WordDocument流的`0x000A`开始的两个字节组成的`ushort`型的`0x0200`位标识了`clx`在`0Table`和`1Table`中的哪个流中（假设它在`table_stream`流中），WordDocument流的`0x01A2`开始的四个字节组成的uint是`clxOffset`，WordDocument流的`0x01A6`开始的四个字节组成的uint是`clxLength`，`clxOffset`表示clx在`table_stream`中的位置，`clxLength`表示`clx`占用几个字节。


### 根据`clx`得到`piece_table`

`clx`中的结构如下，但是注意Prc数组和Pcdt数组的长度不固定，需要通过一定的计算得到需要的数据：

    Clx {
     Variable Prc[] { // 若干个Prc的数组
       1byte clxt; // 一定是0x01
       Variable PrcData { // 这个结构在这里没有用，需要跳过
         2bytes cbGrpprl; // 指明GrpPrl的大小，单位为字节
         Variable GrpPrl;
       }
     }
     Variable Pcdt {
       1byte clxt; // 一定是0x02
       4bytes lcb; // PlcPcd的长度
       Variable PlcPcd { // 用来描述所有文本块的两个数组，又叫我们叫它piece_table
         ... // 内部结构，在下文详细介绍
       }
     }
    }

`clx`数组中保存了多个结构，每个结构的第一个字节只能是`1`或`2`，表示了这个结构的类型；然后的`2`个字节或`4`个字节表示了这个结构的占用了多少字节，然后若干字节保存了这个结构。类型为2的结构就是要找的`piece_table`。

根据上表，我们可以用如下过程得到`piece_table`数组：

    pos = 0;
    while (1) {
      type = get_uchar(clx, pos);
      if (type == 1) {
        pos += 3 + get_ushort(clx, pos + 1);
      } else if (type == 2) {
        piece_table_length = get_uint(clx, pos + 1);
        pice_table = clx[(pos+5) ... (pos+5+piece_table_length)];
        break;
      } else {
        // clx data error
      }
    }


### 根据`piece_table`从WordDocumentStream中提取文本

`piece_table`的结构如下，其中`n`需要通过Pcdt.lcb计算得到，`n=(Pcdt.lcb-4)/12`：

     Variable PlcPcd { // piece_table
       Variable CP[n+1]; // n+1个CP
       Variable Pcd[n]; {// n个Pcd
         1bit fNoParaLast; // 如果是1，文本块一定没有段标记
         1bit fR1; // 未定义
         1bit fDirty; // 一定是0
         13bits fR2; // 未定义
         4bytes fcCompressed; // 文本块在WordDocument Stream中的位置
         2bytes prm; // 用来描述直接段格式和直接字格式
       }
     }

`piece_table`本身包含两个数组：

  * 第一个数组包含`n+1`个字符的逻辑位置（`n`是片段piece的数量），逻辑位置是指字符在文本中是第几个，比如第一个元素是第一个片段在整个文本中的起始位置，第二个元素是第一个片段在文本中的结束位置、也是第二个片段在文本中的开始位置
  * 第二个数组包含了n个片段描述结构（piece descriptor structure）

其中fcCompressed结构如下：

    32bits fcCompressed {
      // 如果fCompressed=0，文本块是一个从fc开始的16-bit的Unicode的字符数组（UTF-16）
      // 如果fCompressed=1，文本块是一个从fc/2开始的8-bit的Unicode的字符数组（其实是ANSI编码）
      //     除了map[]中列出来的例外，需要查询微软提供的表，列在下面
      30bits fc { // 低30位
      }
      [30] bit fCompressed // 表示当前文本块是否被压缩过
      [31] bit r1; // 一定是0
    }

`map`中需要用到的项目如下：

    uint16_t map[256] = {};
    map[0x82] = 0x201A;
    map[0x83] = 0x0192;
    map[0x84] = 0x201E;
    map[0x85] = 0x2026;
    map[0x86] = 0x2020;
    map[0x87] = 0x2021;
    map[0x88] = 0x02C6;
    map[0x89] = 0x2030;
    map[0x8A] = 0x0160;
    map[0x8B] = 0x2039;
    map[0x8C] = 0x0152;
    map[0x91] = 0x2018;
    map[0x92] = 0x2019;
    map[0x93] = 0x201C;
    map[0x94] = 0x201D;
    map[0x95] = 0x2022;
    map[0x96] = 0x2013;
    map[0x97] = 0x2014;
    map[0x98] = 0x02DC;
    map[0x99] = 0x2122;
    map[0x9A] = 0x0161;
    map[0x9B] = 0x203A;
    map[0x9C] = 0x0153;
    map[0x9F] = 0x0178;

其实fcCompressed=1时文本块的编码是ANSI+给定map表，需要根据不同的语言进行转换。但是在这里我们只考虑中文编码，所以只有UTF-16编码可能出现。

根据以上信息，我们可以得到如下提取文本的过程：

    piece_count = (piece_table.length-4)/12;
    for (i = 0; i < piece_count; ++i) {
      cp_start = get_uint(piece_table, i * 4);
      cp_end = get_uint(piece_table, (i+1) * 4);
      cb = cp_end – cp_start;
      if (((fcCompressed >> 30) & 1) == 0) {
        cb *= 2;
        text.append(WordDocumentStream[fc, fc+cb]);
      } else {
        fc = (fcCompressed & ~(1U << 30)) / 2;
        text.append(ChangeEncoding(WordDocumentStream[fc, fc+cb]));
      }
    }

