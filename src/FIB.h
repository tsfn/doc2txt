// WordDocument流从偏移量为0处是结构FIB(File Information Block)
// WordDocument的其他部分都是不是固定偏移的。

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

struct FibBase { // {{{
  ushort wIdent;		// 用来标记的字段，一定是0xA5EC
  ushort nFib;		  // 指定文档版本号，被FibRgCswNew.nFibNew取代
  ushort unused;		// 未定义
  ushort lid;		    // 标记创建本文档的应用程序支持的语言
  ushort pnNext;	  // 如果文档中包括AutoText项目
                    //  pnNext指明了它在WordDocument流中的位置

  ushort fDot: 1; // 标志文件是否是文档模板（document template）
  ushort fGlsy: 1; // 标志文件是否只包括AutoText项目
  ushort fComplex: 1; // 标志文件的最后一次操作是否是
                      //  增量保存（incremental save）
  ushort fHasPic: 1; // 如果是0，文件中应该没有图片  uchar envr;
  ushort cQuickSaves: 4; // 如果nFib小于0x00D9，
                          //  cQuickSaves是文件连续增量保存的次数；
                          //  如果nFib大于等于0x00D9，cQuickSaves一定是0xF
  ushort fEncrypted: 1; // 标志文件是否被加密或被扰乱（obfuscated）
  ushort fWhichTblStm: 1; // 标志FIB引用的是1Table(fWhichTblStm=1)
                          //  还是0Table(fWhichTblStm=0)。
  ushort fReadOnlyRecommended: 1; // 标志文件是否被作者设置为建议只读
  ushort fWriteReservation: 1; // 标记文件是否有写入密码
  ushort fExtChar: 1;     // 一定是1
  ushort fLoadOverride: 1; // 标志是否要用应用程序使用的语言和格式
  ushort fFarEast: 1;     // 标志应用程序使用的语言是否是东亚语言
  ushort fObfuscated: 1; // 如果fEncrypted=1，
                         //  fObfuscated标志了文件是否用XOR扰乱过

  ushort nFibBack;	// 值只可能是0x00BF或0x00C1
  uint IKey;		    // fEncrypted=1且fObfuscation=1时它个验证值，
                    //  fEncrypted=1且fObfuscation=0时它是
                    //  EncryptionHeader的大小，否则它一定是0。
  uchar envr;		    // 一定是0

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
}; // }}}
struct FibRgW97 { // {{{
  ushort reserved1;
  ushort reserved2;
  ushort reserved3;
  ushort reserved4;
  ushort reserved5;
  ushort reserved6;
  ushort reserved7;
  ushort reserved8;
  ushort reserved9;
  ushort reserved10;
  ushort reserved11;
  ushort reserved12;
  ushort reserved13;
  ushort lidFE;
}; // }}}
struct FibRgLw97 { // {{{
  uint cbMac; // WordDocument流从cbMac位置开始的所有字节是无效的
  uint reserved1;
  uint reserved2;
  int ccpText; // 值必须>=0，main document中CP的个数
  int ccpFtn; // 值必须>=0，footnote subdocument中CP的个数
  int ccpHdd; // 值必须>=0，header subdocument中CP的个数
  uint reserved3;
  int ccpAtn; // 值必须>=0，comment subdocument中CP的个数
  int ccpEdn; // 值必须>=0， endnote subdocument中CP的个数
  int ccpTxbx; // 值必须>=0，maindocument中的textbox subdocument中CP的个数
  int ccpHdrTxbx; // 值必须>=0，header中的textbox subdocument中CP的个数
  uint reserved4;
  uint reserved5;
  uint reserved6;
  uint reserved7;
  uint reserved8;
  uint reserved9;
  uint reserved10;
  uint reserved11;
  uint reserved12;
  uint reserved13;
  uint reserved14;
}; // }}}
struct FibRgFcLcb97 { // {{{
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
  uint fcClx;
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
}; // }}}
struct FibRgFcLcb2000 { // {{{
  struct FibRgFcLcb97 rgFcLcb97; // 744 bytes
  uint fcPlcfTch;
  uint lcbPlcfTch;
  uint fcRmdThreading;
  uint lcbRmdThreading;
  uint fcMid;
  uint lcbMid;
  uint fcSttbRgtplc;
  uint lcbSttbRgtplc;
  uint fcMsoEnvelope;
  uint lcbMsoEnvelope;
  uint fcPlcfLad;
  uint lcbPlcfLad;
  uint fcRgDofr;
  uint lcbRgDofr;
  uint fcPlcosl;
  uint lcbPlcosl;
  uint fcPlcfCookieOld;
  uint lcbPlcfCookieOld;
  uint fcPgdMotherOld;
  uint lcbPgdMotherOld;
  uint fcBkdMotherOld;
  uint lcbBkdMotherOld;
  uint fcPgdFtnOld;
  uint lcbPgdFtnOld;
  uint fcBkdFtnOld;
  uint lcbBkdFtnOld;
  uint fcPgdEdnOld;
  uint lcbPgdEdnOld;
  uint fcBkdEdnOld;
  uint lcbBkdEdnOld;
}; // }}}
struct FibRgFcLcb2002 { // {{{
  struct FibRgFcLcb2000 rgFcLcb2000; // 864 bytes
  uint fcUnused1;
  uint lcbUnused1;
  uint fcPlcfPgp;
  uint lcbPlcfPgp;
  uint fcPlcfuim;
  uint lcbPlcfuim;
  uint fcPlfguidUim;
  uint lcbPlfguidUim;
  uint fcAtrdExtra;
  uint lcbAtrdExtra;
  uint fcPlrsid;
  uint lcbPlrsid;
  uint fcSttbfBkmkFactoid;
  uint lcbSttbfBkmkFactoid;
  uint fcPlcfBkfFactoid;
  uint lcbPlcfBkfFactoid;
  uint fcPlcfcookie;
  uint lcbPlcfcookie;
  uint fcPlcfBklFactoid;
  uint lcbPlcfBklFactoid;
  uint fcFactoidData;
  uint lcbFactoidData;
  uint fcDocUndo;
  uint lcbDocUndo;
  uint fcSttbfBkmkFcc;
  uint lcbSttbfBkmkFcc;
  uint fcPlcfBkfFcc;
  uint lcbPlcfBkfFcc;
  uint fcPlcfBklFcc;
  uint lcbPlcfBklFcc;
  uint fcSttbfbkmkBPRepairs;
  uint lcbSttbfbkmkBPRepairs;
  uint fcPlcfbkfBPRepairs;
  uint lcbPlcfbkfBPRepairs;
  uint fcPlcfbklBPRepairs;
  uint lcbPlcfbklBPRepairs;
  uint fcPmsNew;
  uint lcbPmsNew;
  uint fcODSO;
  uint lcbODSO;
  uint fcPlcfpmiOldXP;
  uint lcbPlcfpmiOldXP;
  uint fcPlcfpmiNewXP;
  uint lcbPlcfpmiNewXP;
  uint fcPlcfpmiMixedXP;
  uint lcbPlcfpmiMixedXP;
  uint fcUnused2;
  uint lcbUnused2;
  uint fcPlcffactoid;
  uint lcbPlcffactoid;
  uint fcPlcflvcOldXP;
  uint lcbPlcflvcOldXP;
  uint fcPlcflvcNewXP;
  uint lcbPlcflvcNewXP;
  uint fcPlcflvcMixedXP;
  uint lcbPlcflvcMixedXP;
}; // }}}
struct FibRgFcLcb2003 { // {{{
  struct FibRgFcLcb2002 rgFcLcb2002; // 1088 bytes
  uint fcHplxsdr;
  uint lcbHplxsdr;
  uint fcSttbfBkmkSdt;
  uint lcbSttbfBkmkSdt;
  uint fcPlcfBkfSdt;
  uint lcbPlcfBkfSdt;
  uint fcPlcfBklSdt;
  uint lcbPlcfBklSdt;
  uint fcCustomXForm;
  uint lcbCustomXForm;
  uint fcSttbfBkmkProt;
  uint lcbSttbfBkmkProt;
  uint fcPlcfBkfProt;
  uint lcbPlcfBkfProt;
  uint fcPlcfBklProt;
  uint lcbPlcfBklProt;
  uint fcSttbProtUser;
  uint lcbSttbProtUser;
  uint fcUnused;
  uint lcbUnused;
  uint fcPlcfpmiOld;
  uint lcbPlcfpmiOld;
  uint fcPlcfpmiOldInline;
  uint lcbPlcfpmiOldInline;
  uint fcPlcfpmiNew;
  uint lcbPlcfpmiNew;
  uint fcPlcfpmiNewInline;
  uint lcbPlcfpmiNewInline;
  uint fcPlcflvcOld;
  uint lcbPlcflvcOld;
  uint fcPlcflvcOldInline;
  uint lcbPlcflvcOldInline;
  uint fcPlcflvcNew;
  uint lcbPlcflvcNew;
  uint fcPlcflvcNewInline;
  uint lcbPlcflvcNewInline;
  uint fcPgdMother;
  uint lcbPgdMother;
  uint fcBkdMother;
  uint lcbBkdMother;
  uint fcAfdMother;
  uint lcbAfdMother;
  uint fcPgdFtn;
  uint lcbPgdFtn;
  uint fcBkdFtn;
  uint lcbBkdFtn;
  uint fcAfdFtn;
  uint lcbAfdFtn;
  uint fcPgdEdn;
  uint lcbPgdEdn;
  uint fcBkdEdn;
  uint lcbBkdEdn;
  uint fcAfdEdn;
  uint lcbAfdEdn;
  uint fcAfd;
  uint lcbAfd;
}; // }}}
struct FibRgFcLcb2007 { // {{{
  struct FibRgFcLcb2003 rgFcLcb2003; // 1312 bytes
  uint fcPlcfmthd;
  uint lcbPlcfmthd;
  uint fcSttbfBkmkMoveFrom;
  uint lcbSttbfBkmkMoveFrom;
  uint fcPlcfBkfMoveFrom;
  uint lcbPlcfBkfMoveFrom;
  uint fcPlcfBklMoveFrom;
  uint lcbPlcfBklMoveFrom;
  uint fcSttbfBkmkMoveTo;
  uint lcbSttbfBkmkMoveTo;
  uint fcPlcfBkfMoveTo;
  uint lcbPlcfBkfMoveTo;
  uint fcPlcfBklMoveTo;
  uint lcbPlcfBklMoveTo;
  uint fcUnused1;
  uint lcbUnused1;
  uint fcUnused2;
  uint lcbUnused2;
  uint fcUnused3;
  uint lcbUnused3;
  uint fcSttbfBkmkArto;
  uint lcbSttbfBkmkArto;
  uint fcPlcfBkfArto;
  uint lcbPlcfBkfArto;
  uint fcPlcfBklArto;
  uint lcbPlcfBklArto;
  uint fcArtoData;
  uint lcbArtoData;
  uint fcUnused4;
  uint lcbUnused4;
  uint fcUnused5;
  uint lcbUnused5;
  uint fcUnused6;
  uint lcbUnused6;
  uint fcOssTheme;
  uint lcbOssTheme;
  uint fcColorSchemeMapping;
  uint lcbColorSchemeMapping;
}; // }}}
struct FibRgCswNewData2000 { // {{{
  ushort cQuickSavesNew;  // 该文档在最后一次完全保存后进行了多少次增量保存
                          //  取值范围必须在0到0x000F之间
}; // }}}
struct FibRgCswNewData2007 { // {{{
  struct FibRgCswNewData2000 rgCswNewData2000;
  ushort lidThemeOther; // 未定义
  ushort lidThemeFE; // 未定义
  ushort idThemeCS; // 未定义
}; // }}}
struct FibRgCswNew { // {{{
  ushort nFibNew; // 文件格式的版本号，它的值一定是以下值中的一个
                  //  0x00D9
                  //  0x0101
                  //  0x010C
                  //  0x0112
  struct FibRgCswNewData2000 rgCswNewData; // 根据nFibNew的值，取以下值中的一个
                  // nFibNew    类型
                  // 0x00D9     fibRgCswNewData2000 (2 bytes)
                  // 0x0101     fibRgCswNewData2000 (2 bytes)
                  // 0x010C     fibRgCswNewData2000 (2 bytes)
                  // 0x0112     fibRgCswNewData2007 (8 bytes)
}; // }}}
struct FibRgFcLcb { // {{{
  struct FibRgFcLcb97 FibRgFcLcb97; // 根据FibBase.nFib而定
}; // }}}



struct Fib {
  struct FibBase base; // 32 bytes

  ushort csw; // 一定是0x000E，表示fibRgw占用多少个ushort单位（2字节）
  struct FibRgW97 fibRgw; // 28 bytes

  ushort cslw; // 一定是0x0016，表示fibRgLw占用多少个uint单位（4字节）
  struct FibRgLw97 fibRgLw; // 88 bytes

  ushort cbRgFcLcb; // 表示fibRgFcLcbBlob占用了多少个8字节
                    // 根据FibBase.nFib而定，一定是以下取值之一
                    //  nFib      cbRgFcLcb
                    //  0x00C1    0x005D
                    //  0x00D9    0x006C
                    //  0x0101    0x0088
                    //  0x010C    0x00A4
                    //  0x0112    0x00B7
  struct FibRgFcLcb fibRgFcLcbBlob; // 变长

  ushort cswNew;    // fibRgCswNew占用多少个ushort单位（2字节）
                    // 根据FibBase.nFib而定，一定是以下取值之一
                    //  nFib      cswNew
                    //  0x00C1    0
                    //  0x00D9    0x0002
                    //  0x0101    0x0002
                    //  0x010C    0x0002
                    //  0x0112    0x0005
  struct FibRgCswNew fibRgCswNew; // 变长
};

