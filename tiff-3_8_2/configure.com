$! Configure procedure 
$! (c) Alexey Chupahin  18-APR-2006
$! 
$!
$!
$! HAVE_JPEG,  HAVE_JPEG_SHARED   -- test for libjpeg  JPEG_LIBRARY_PATH - a path of library to be used
$! HAVE_ZLIB,  HAVE_ZLIB_SHARED  -- test for libz
$! HAVE_LFIND, HAVE_STRCASECMP   -- correspond libraries
$! 
$!
$ SET NOON
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Configuring libTIFF library"
$WRITE SYS$OUTPUT " "
$! Checking architecture
$DECC = F$SEARCH("SYS$SYSTEM:DECC$COMPILER.EXE") .NES. ""
$    IF F$GETSYI("ARCH_TYPE").EQ.1 THEN CPU = "VAX"
$    IF F$GETSYI("ARCH_TYPE").EQ.2 THEN CPU = "Alpha"
$    IF F$GETSYI("ARCH_TYPE").EQ.3 THEN CPU = "I64"
$WRITE SYS$OUTPUT "Checking architecture 	...  ", CPU
$SHARED=0
$IF ( (CPU.EQS."Alpha").OR.(CPU.EQS."I64") )
$  THEN 
$	SHARED=64
$  ELSE
$	SHARED=32
$ENDIF
$IF (DECC) THEN $WRITE SYS$OUTPUT  "Compiler		...  DEC C"
$IF (.NOT. DECC) THEN $WRITE SYS$OUTPUT  "BAD compiler" GOTO EXIT
$MMS = F$SEARCH("SYS$SYSTEM:MMS.EXE") .NES. ""
$MMK = F$TYPE(MMK) 
$IF (MMS .OR. MMK) THEN GOTO TEST_LIBRARIES
$! I cant find any make tool
$ WRITE SYS$OUTPUT "Install MMS or MMK"
$GOTO EXIT
$!
$!
$TEST_LIBRARIES:
$!   Setting as MAKE utility one of MMS or MMK. I prefer MMS.
$IF (MMK) THEN MAKE="MMK"
$IF (MMS) THEN MAKE="MMS"
$WRITE SYS$OUTPUT "Checking build utility	...  ''MAKE'"
$WRITE SYS$OUTPUT " "
$!
$!
$!"Checking for strcasecmp "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
	#include  <strings.h>
	#include  <stdlib.h>

    int main()
	{
        if (strcasecmp("bla", "Bla")==0) exit(0);
	   else exit(2);
	}
$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       HAVE_STRCASECMP=0
$       GOTO NEXT1
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_STRCASECMP=0
$       GOTO NEXT1
$ENDIF
$!
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$RUN TEST
$IF ($STATUS .NE. %X00000001)
$  THEN
$	HAVE_STRCASECMP=0
$  ELSE
$	 HAVE_STRCASECMP=1
$ENDIF
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$NEXT1:
$IF (HAVE_STRCASECMP.EQ.1)
$  THEN
$ 	WRITE SYS$OUTPUT "Checking for strcasecmp ...   Yes"	
$  ELSE
$	WRITE SYS$OUTPUT "Checking for strcasecmp ...   No"
$ENDIF
$!
$!

$!"Checking for lfind "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
        #include  <search.h>

    int main()
        {
        lfind((const void *)key, (const void *)NULL, (size_t *)NULL,
           (size_t) 0, NULL);
        }
$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       HAVE_LFIND=0
$       GOTO NEXT2
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_LFIND=0
$       GOTO NEXT2
$  ELSE
$        HAVE_LFIND=1
$ENDIF
$!
$NEXT2:
$IF (HAVE_LFIND.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for lfind ...   Yes"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for lfind ...   No"
$ENDIF
$!
$!
$!
$!"Checking for correct zlib library    "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <zlib.h>
   int main()
     {
	printf("checking version zlib:  %s\n",zlibVersion());
     }
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10B90001) 
$  THEN 
$	HAVE_ZLIB=0
$	GOTO EXIT
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,ZLIB:LIBZ/LIB 
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001) 
$  THEN 
$	HAVE_ZLIB=0
$       GOTO EXIT
$  ELSE
$	HAVE_ZLIB=1
$ENDIF
$IF (HAVE_ZLIB.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for correct zlib library ...   Yes"
$  ELSE
$	WRITE SYS$OUTPUT "Checking for correct zlib library ...   No"
$       WRITE SYS$OUTPUT "This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$ENDIF
$RUN TEST
$!
$! Checking for JPEG ...
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(JPEG) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <jpeglib.h>
      #include <jversion.h>	
   int main()
     {
	printf("checking version jpeg:  %s\n",JVERSION);
	jpeg_quality_scaling(0);
        return 0;
     }
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10B90001)
$  THEN
$       WRITE SYS$OUTPUT "Checking for static jpeg library ...   No"
$	HAVE_JPEG=0
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,JPEG:LIBJPEG/LIB
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$	HAVE_JPEG=0
$  ELSE
$	HAVE_JPEG=1
$ENDIF
$IF (HAVE_JPEG.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for static jpeg library ...   Yes"
$	JPEG_LIBRARY_PATH="JPEG:LIBJPEG/LIB"
$	RUN TEST
$  ELSE
$       WRITE SYS$OUTPUT "Checking for static jpeg library ...   No"
$ENDIF
$!
$!"Checking for SHARED JPEG library    "
$OPEN/WRITE OUT TEST.OPT
$WRITE OUT "SYS$SHARE:LIBJPEG$SHR/SHARE"
$WRITE OUT "ZLIB:LIBZ/LIB"
$CLOSE OUT
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,TEST/OPT
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_JPEG_SHARED=0
$  ELSE
$       HAVE_JPEG_SHARED=1
$ENDIF
$IF (HAVE_JPEG_SHARED.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for shared jpeg library ...   Yes"
$	JPEG_LIBRARY_PATH="SYS$SHARE:LIBJPEG$SHR/SHARE"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for shared jpeg library ...   No"
$ENDIF
$!
$ IF ( (HAVE_JPEG_SHARED.EQ.0).AND.(HAVE_JPEG.EQ.0) )
$    THEN
$       WRITE SYS$OUTPUT "No JPEG library installed. This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$	GOTO EXIT
$ ENDIF
$!
$! Checking for X11 ...
$IF F$TRNLNM("DECW$INCLUDE") .NES. ""
$  THEN
$	WRITE SYS$OUTPUT "Checking for X11 ...   Yes"
$  ELSE
$	WRITE SYS$OUTPUT "Checking for X11 ...   No"
$	WRITE SYS$OUTPUT "This is fatal. Please install X11 software"
$	GOTO EXIT
$ENDIF
$!
$!WRITING BUILD FILES
$OPEN/WRITE OUT BUILD.COM
$ WRITE OUT "$set def [.PORT]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-.libtiff]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-.tools]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-]"
$ WRITE OUT "$cop [.PORT]LIBPORT.OLB [.LIBTIFF]LIBPORT.OLB"
$ WRITE OUT "$ CURRENT = F$ENVIRONMENT (""DEFAULT"") "
$ WRITE OUT "$TIFF=CURRENT"
$ WRITE OUT "$OPEN/WRITE OUTT LIBTIFF$STARTUP.COM"
$ WRITE OUT "$TIFF[F$LOCATE(""]"",TIFF),9]:="".LIBTIFF]"""
$ WRITE OUT "$WRITE OUTT ""DEFINE TIFF ","'","'","TIFF'""
$ WRITE OUT "$TIFF=CURRENT"
$ WRITE OUT "$TIFF[F$LOCATE(""]"",TIFF),7]:="".TOOLS]"""
$ WRITE OUT "$WRITE OUTT ""BMP2TIFF:==$", "'","'","TIFF'BMP2TIFF"""
$ WRITE OUT "$WRITE OUTT ""FAX2PS:==$", "'","'","TIFF'FAX2PS"""
$ WRITE OUT "$WRITE OUTT ""FAX2TIFF:==$", "'","'","TIFF'FAX2TIFF"""
$ WRITE OUT "$WRITE OUTT ""GIF2TIFF:==$", "'","'","TIFF'GIF2TIFF"""
$ WRITE OUT "$WRITE OUTT ""PAL2RGB:==$", "'","'","TIFF'PAL2RGB"""
$ WRITE OUT "$WRITE OUTT ""PPM2TIFF:==$", "'","'","TIFF'PPM2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RAS2TIFF:==$", "'","'","TIFF'RAS2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RAW2TIFF:==$", "'","'","TIFF'RAW2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RGB2YCBCR:==$", "'","'","TIFF'RGB2YCBCR"""
$ WRITE OUT "$WRITE OUTT ""THUMBNAIL:==$", "'","'","TIFF'THUMBNAIL"""
$ WRITE OUT "$WRITE OUTT ""TIFF2BW:==$", "'","'","TIFF'TIFF2BW"""
$ WRITE OUT "$WRITE OUTT ""TIFF2PDF:==$", "'","'","TIFF'TIFF2PDF"""
$ WRITE OUT "$WRITE OUTT ""TIFF2PS:==$", "'","'","TIFF'TIFF2PS"""
$ WRITE OUT "$WRITE OUTT ""TIFF2RGBA:==$", "'","'","TIFF'TIFF2RGBA"""
$ WRITE OUT "$WRITE OUTT ""TIFFCMP:==$", "'","'","TIFF'TIFFCMP"""
$ WRITE OUT "$WRITE OUTT ""TIFFCP:==$", "'","'","TIFF'TIFFCP"""
$ WRITE OUT "$WRITE OUTT ""TIFFDITHER:==$", "'","'","TIFF'TIFFDITHER"""
$ WRITE OUT "$WRITE OUTT ""TIFFDUMP:==$", "'","'","TIFF'TIFFDUMP"""
$ WRITE OUT "$WRITE OUTT ""TIFFINFO:==$", "'","'","TIFF'TIFFINFO"""
$ WRITE OUT "$WRITE OUTT ""TIFFMEDIAN:==$", "'","'","TIFF'TIFFMEDIAN"""
$ WRITE OUT "$CLOSE OUTT"
$ WRITE OUT "$OPEN/WRITE OUTT [.LIBTIFF]LIBTIFF.OPT"
$  IF (SHARED.GT.0)
$    THEN
$	WRITE OUT "$WRITE OUTT ""SYS$SHARE:TIFF$SHR/SHARE""
$    ELSE
$ 	WRITE OUT "$WRITE OUTT ""TIFF:TIFF/LIB""
$ 	WRITE OUT "$WRITE OUTT ""TIFF:LIBPORT/LIB""
$  ENDIF
$   IF (HAVE_JPEG_SHARED.EQ.1)
$     THEN
$	WRITE OUT "$WRITE OUTT ""SYS$SHARE:LIBJPEG$SHR/SHARE""
$     ELSE
$ 	WRITE OUT "$WRITE OUTT ""JPEG:LIBJPEG/LIB""
$   ENDIF
$ WRITE OUT "$WRITE OUTT ""ZLIB:LIBZ/LIB""
$ WRITE OUT "$CLOSE OUTT"
$!
$ WRITE OUT "$WRITE SYS$OUTPUT "" ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""***************************************************************************** ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""LIBTIFF$STARTUP.COM has been created. ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""This file setups all logicals needed. It should be execute before using LibTIFF ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""Nice place to call it - LOGIN.COM ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""***************************************************************************** ""
$CLOSE OUT
$!
$! DESCRIP.MMS in [.PORT]
$OBJ="DUMMY.OBJ"
$IF HAVE_STRCASECMP.NE.1 
$  THEN 
$     OBJ=OBJ+",STRCASECMP.OBJ"
$ENDIF
$IF HAVE_LFIND.NE.1   
$   THEN 
$       OBJ=OBJ+",LFIND.OBJ"
$ENDIF
$OPEN/WRITE OUT [.PORT]DESCRIP.MMS
$WRITE OUT "CFLAGS=/OPT=(INLINE=SPEED)"
$WRITE OUT ""
$WRITE OUT "OBJ=",OBJ
$WRITE OUT ""
$WRITE OUT "LIBPORT.OLB : $(OBJ)"
$WRITE OUT "	LIB/CREA LIBPORT $(OBJ)"
$WRITE OUT ""
$CLOSE OUT
$!
$! Writing TIFF$DEF.OPT
$ IF (SHARED.EQ.32)
$   THEN
$	COPY SYS$INPUT TIFF$DEF.OPT
        UNIVERSAL=TIFFOpen
        UNIVERSAL=TIFFGetVersion
        UNIVERSAL=TIFFCleanup
        UNIVERSAL=TIFFClose
        UNIVERSAL=TIFFFlush
        UNIVERSAL=TIFFFlushData
        UNIVERSAL=TIFFGetField
        UNIVERSAL=TIFFVGetField
        UNIVERSAL=TIFFGetFieldDefaulted
        UNIVERSAL=TIFFVGetFieldDefaulted
        UNIVERSAL=TIFFGetTagListEntry
        UNIVERSAL=TIFFGetTagListCount
        UNIVERSAL=TIFFReadDirectory
        UNIVERSAL=TIFFScanlineSize
        UNIVERSAL=TIFFStripSize
        UNIVERSAL=TIFFVStripSize
        UNIVERSAL=TIFFRawStripSize
        UNIVERSAL=TIFFTileRowSize
        UNIVERSAL=TIFFTileSize
        UNIVERSAL=TIFFVTileSize
        UNIVERSAL=TIFFFileno
        UNIVERSAL=TIFFSetFileno
        UNIVERSAL=TIFFGetMode
        UNIVERSAL=TIFFIsTiled
        UNIVERSAL=TIFFIsByteSwapped
        UNIVERSAL=TIFFIsBigEndian
        UNIVERSAL=TIFFIsMSB2LSB
        UNIVERSAL=TIFFIsUpSampled
        UNIVERSAL=TIFFCIELabToRGBInit
        UNIVERSAL=TIFFCIELabToXYZ
        UNIVERSAL=TIFFXYZToRGB
        UNIVERSAL=TIFFYCbCrToRGBInit
        UNIVERSAL=TIFFYCbCrtoRGB
        UNIVERSAL=TIFFCurrentRow
        UNIVERSAL=TIFFCurrentDirectory
        UNIVERSAL=TIFFCurrentStrip
        UNIVERSAL=TIFFCurrentTile
        UNIVERSAL=TIFFDataWidth
        UNIVERSAL=TIFFReadBufferSetup
        UNIVERSAL=TIFFWriteBufferSetup
        UNIVERSAL=TIFFSetupStrips
        UNIVERSAL=TIFFLastDirectory
        UNIVERSAL=TIFFSetDirectory
        UNIVERSAL=TIFFSetSubDirectory
        UNIVERSAL=TIFFUnlinkDirectory
        UNIVERSAL=TIFFSetField
        UNIVERSAL=TIFFVSetField
        UNIVERSAL=TIFFCheckpointDirectory
        UNIVERSAL=TIFFWriteDirectory
        UNIVERSAL=TIFFRewriteDirectory
        UNIVERSAL=TIFFPrintDirectory
        UNIVERSAL=TIFFReadScanline
        UNIVERSAL=TIFFWriteScanline
        UNIVERSAL=TIFFReadRGBAImage
        UNIVERSAL=TIFFReadRGBAImageOriented
        UNIVERSAL=TIFFFdOpen
        UNIVERSAL=TIFFClientOpen
        UNIVERSAL=TIFFFileName
        UNIVERSAL=TIFFError
        UNIVERSAL=TIFFErrorExt
        UNIVERSAL=TIFFWarning
        UNIVERSAL=TIFFWarningExt
        UNIVERSAL=TIFFSetErrorHandler
        UNIVERSAL=TIFFSetErrorHandlerExt
        UNIVERSAL=TIFFSetWarningHandler
        UNIVERSAL=TIFFSetWarningHandlerExt
        UNIVERSAL=TIFFComputeTile
        UNIVERSAL=TIFFCheckTile
        UNIVERSAL=TIFFNumberOfTiles
        UNIVERSAL=TIFFReadTile
        UNIVERSAL=TIFFWriteTile
        UNIVERSAL=TIFFComputeStrip
        UNIVERSAL=TIFFNumberOfStrips
        UNIVERSAL=TIFFRGBAImageBegin
        UNIVERSAL=TIFFRGBAImageGet
        UNIVERSAL=TIFFRGBAImageEnd
        UNIVERSAL=TIFFReadEncodedStrip
        UNIVERSAL=TIFFReadRawStrip
        UNIVERSAL=TIFFReadEncodedTile
        UNIVERSAL=TIFFReadRawTile
        UNIVERSAL=TIFFReadRGBATile
        UNIVERSAL=TIFFReadRGBAStrip
        UNIVERSAL=TIFFWriteEncodedStrip
        UNIVERSAL=TIFFWriteRawStrip
        UNIVERSAL=TIFFWriteEncodedTile
        UNIVERSAL=TIFFWriteRawTile
        UNIVERSAL=TIFFSetWriteOffset
        UNIVERSAL=TIFFSwabDouble
        UNIVERSAL=TIFFSwabShort
        UNIVERSAL=TIFFSwabLong
        UNIVERSAL=TIFFSwabArrayOfShort
        UNIVERSAL=TIFFSwabArrayOfLong
        UNIVERSAL=TIFFSwabArrayOfDouble
        UNIVERSAL=TIFFSwabArrayOfTriples
        UNIVERSAL=TIFFReverseBits
        UNIVERSAL=TIFFGetBitRevTable
        UNIVERSAL=TIFFDefaultStripSize
        UNIVERSAL=TIFFDefaultTileSize
        UNIVERSAL=TIFFRasterScanlineSize
        UNIVERSAL=_TIFFmalloc
        UNIVERSAL=_TIFFrealloc
        UNIVERSAL=_TIFFfree
        UNIVERSAL=_TIFFmemset
        UNIVERSAL=_TIFFmemcpy
        UNIVERSAL=_TIFFmemcmp
        UNIVERSAL=TIFFCreateDirectory
        UNIVERSAL=TIFFSetTagExtender
        UNIVERSAL=TIFFMergeFieldInfo
        UNIVERSAL=TIFFFindFieldInfo
        UNIVERSAL=TIFFFindFieldInfoByName
        UNIVERSAL=TIFFFieldWithName
        UNIVERSAL=TIFFFieldWithTag
        UNIVERSAL=TIFFCurrentDirOffset
        UNIVERSAL=TIFFWriteCheck
        UNIVERSAL=TIFFRGBAImageOK
        UNIVERSAL=TIFFNumberOfDirectories
        UNIVERSAL=TIFFSetFileName
        UNIVERSAL=TIFFSetClientdata
        UNIVERSAL=TIFFSetMode
        UNIVERSAL=TIFFClientdata
        UNIVERSAL=TIFFGetReadProc
        UNIVERSAL=TIFFGetWriteProc
        UNIVERSAL=TIFFGetSeekProc
        UNIVERSAL=TIFFGetCloseProc
        UNIVERSAL=TIFFGetSizeProc
        UNIVERSAL=TIFFGetMapFileProc
        UNIVERSAL=TIFFGetUnmapFileProc
        UNIVERSAL=TIFFIsCODECConfigured
        UNIVERSAL=TIFFGetConfiguredCODECs
        UNIVERSAL=TIFFFindCODEC
        UNIVERSAL=TIFFRegisterCODEC
        UNIVERSAL=TIFFUnRegisterCODEC
        UNIVERSAL=TIFFFreeDirectory
        UNIVERSAL=TIFFReadCustomDirectory
        UNIVERSAL=TIFFReadEXIFDirectory
        UNIVERSAL=TIFFAccessTagMethods
        UNIVERSAL=TIFFGetClientInfo
        UNIVERSAL=TIFFSetClientInfo
        UNIVERSAL=TIFFReassignTagToIgnore
$ ENDIF
$ IF (SHARED.EQ.64)
$   THEN
$       COPY SYS$INPUT TIFF$DEF.OPT
SYMBOL_VECTOR= (-
        TIFFOpen=PROCEDURE,-
        TIFFGetVersion=PROCEDURE,-
        TIFFCleanup=PROCEDURE,-
        TIFFClose=PROCEDURE,-
        TIFFFlush=PROCEDURE,-
        TIFFFlushData=PROCEDURE,-
        TIFFGetField=PROCEDURE,-
        TIFFVGetField=PROCEDURE,-
        TIFFGetFieldDefaulted=PROCEDURE,-
        TIFFVGetFieldDefaulted=PROCEDURE,-
        TIFFGetTagListEntry=PROCEDURE,-
        TIFFGetTagListCount=PROCEDURE,-
        TIFFReadDirectory=PROCEDURE,-
        TIFFScanlineSize=PROCEDURE,-
        TIFFStripSize=PROCEDURE,-
        TIFFVStripSize=PROCEDURE,-
        TIFFRawStripSize=PROCEDURE,-
        TIFFTileRowSize=PROCEDURE,-
        TIFFTileSize=PROCEDURE,-
        TIFFVTileSize=PROCEDURE,-
        TIFFFileno=PROCEDURE,-
        TIFFSetFileno=PROCEDURE,-
        TIFFGetMode=PROCEDURE,-
        TIFFIsTiled=PROCEDURE,-
        TIFFIsByteSwapped=PROCEDURE,-
        TIFFIsBigEndian=PROCEDURE,-
        TIFFIsMSB2LSB=PROCEDURE,-
        TIFFIsUpSampled=PROCEDURE,-
        TIFFCIELabToRGBInit=PROCEDURE,-
        TIFFCIELabToXYZ=PROCEDURE,-
        TIFFXYZToRGB=PROCEDURE,-
        TIFFYCbCrToRGBInit=PROCEDURE,-
        TIFFYCbCrtoRGB=PROCEDURE,-
        TIFFCurrentRow=PROCEDURE,-
        TIFFCurrentDirectory=PROCEDURE,-
        TIFFCurrentStrip=PROCEDURE,-
        TIFFCurrentTile=PROCEDURE,-
        TIFFDataWidth=PROCEDURE,-
        TIFFReadBufferSetup=PROCEDURE,-
        TIFFWriteBufferSetup=PROCEDURE,-
        TIFFSetupStrips=PROCEDURE,-
        TIFFLastDirectory=PROCEDURE,-
        TIFFSetDirectory=PROCEDURE,-
        TIFFSetSubDirectory=PROCEDURE,-
        TIFFUnlinkDirectory=PROCEDURE,-
        TIFFSetField=PROCEDURE,-
        TIFFVSetField=PROCEDURE,-
        TIFFCheckpointDirectory=PROCEDURE,-
        TIFFWriteDirectory=PROCEDURE,-
        TIFFRewriteDirectory=PROCEDURE,-
        TIFFPrintDirectory=PROCEDURE,-
        TIFFReadScanline=PROCEDURE,-
        TIFFWriteScanline=PROCEDURE,-
        TIFFReadRGBAImage=PROCEDURE,-
        TIFFReadRGBAImageOriented=PROCEDURE,-
        TIFFFdOpen=PROCEDURE,-
        TIFFClientOpen=PROCEDURE,-
        TIFFFileName=PROCEDURE,-
        TIFFError=PROCEDURE,-
        TIFFErrorExt=PROCEDURE,-
        TIFFWarning=PROCEDURE,-
        TIFFWarningExt=PROCEDURE,-
        TIFFSetErrorHandler=PROCEDURE,-
        TIFFSetErrorHandlerExt=PROCEDURE,-
        TIFFSetWarningHandler=PROCEDURE,-
        TIFFSetWarningHandlerExt=PROCEDURE,-
        TIFFComputeTile=PROCEDURE,-
        TIFFCheckTile=PROCEDURE,-
        TIFFNumberOfTiles=PROCEDURE,-
        TIFFReadTile=PROCEDURE,-
        TIFFWriteTile=PROCEDURE,-
        TIFFComputeStrip=PROCEDURE,-
        TIFFNumberOfStrips=PROCEDURE,-
        TIFFRGBAImageBegin=PROCEDURE,-
        TIFFRGBAImageGet=PROCEDURE,-
        TIFFRGBAImageEnd=PROCEDURE,-
        TIFFReadEncodedStrip=PROCEDURE,-
        TIFFReadRawStrip=PROCEDURE,-
        TIFFReadEncodedTile=PROCEDURE,-
        TIFFReadRawTile=PROCEDURE,-
        TIFFReadRGBATile=PROCEDURE,-
        TIFFReadRGBAStrip=PROCEDURE,-
        TIFFWriteEncodedStrip=PROCEDURE,-
        TIFFWriteRawStrip=PROCEDURE,-
        TIFFWriteEncodedTile=PROCEDURE,-
        TIFFWriteRawTile=PROCEDURE,-
        TIFFSetWriteOffset=PROCEDURE,-
        TIFFSwabDouble=PROCEDURE,-
        TIFFSwabShort=PROCEDURE,-
        TIFFSwabLong=PROCEDURE,-
        TIFFSwabArrayOfShort=PROCEDURE,-
        TIFFSwabArrayOfLong=PROCEDURE,-
        TIFFSwabArrayOfDouble=PROCEDURE,-
        TIFFSwabArrayOfTriples=PROCEDURE,-
        TIFFReverseBits=PROCEDURE,-
        TIFFGetBitRevTable=PROCEDURE,-
        TIFFDefaultStripSize=PROCEDURE,-
        TIFFDefaultTileSize=PROCEDURE,-
        TIFFRasterScanlineSize=PROCEDURE,-
        _TIFFmalloc=PROCEDURE,-
        _TIFFrealloc=PROCEDURE,-
        _TIFFfree=PROCEDURE,-
        _TIFFmemset=PROCEDURE,-
        _TIFFmemcpy=PROCEDURE,-
        _TIFFmemcmp=PROCEDURE,-
        TIFFCreateDirectory=PROCEDURE,-
        TIFFSetTagExtender=PROCEDURE,-
        TIFFMergeFieldInfo=PROCEDURE,-
        TIFFFindFieldInfo=PROCEDURE,-
        TIFFFindFieldInfoByName=PROCEDURE,-
        TIFFFieldWithName=PROCEDURE,-
        TIFFFieldWithTag=PROCEDURE,-
        TIFFCurrentDirOffset=PROCEDURE,-
        TIFFWriteCheck=PROCEDURE,-
        TIFFRGBAImageOK=PROCEDURE,-
        TIFFNumberOfDirectories=PROCEDURE,-
        TIFFSetFileName=PROCEDURE,-
        TIFFSetClientdata=PROCEDURE,-
        TIFFSetMode=PROCEDURE,-
        TIFFClientdata=PROCEDURE,-
        TIFFGetReadProc=PROCEDURE,-
        TIFFGetWriteProc=PROCEDURE,-
        TIFFGetSeekProc=PROCEDURE,-
        TIFFGetCloseProc=PROCEDURE,-
        TIFFGetSizeProc=PROCEDURE,-
        TIFFGetMapFileProc=PROCEDURE,-
        TIFFGetUnmapFileProc=PROCEDURE,-
        TIFFIsCODECConfigured=PROCEDURE,-
        TIFFGetConfiguredCODECs=PROCEDURE,-
        TIFFFindCODEC=PROCEDURE,-
        TIFFRegisterCODEC=PROCEDURE,-
        TIFFUnRegisterCODEC=PROCEDURE,-
        TIFFFreeDirectory=PROCEDURE,-
        TIFFReadCustomDirectory=PROCEDURE,-
        TIFFReadEXIFDirectory=PROCEDURE,-
        TIFFAccessTagMethods=PROCEDURE,-
        TIFFGetClientInfo=PROCEDURE,-
        TIFFSetClientInfo=PROCEDURE,-
        TIFFReassignTagToIgnore=PROCEDURE)
$ENDIF
$!
$! Writing TIFF$SHR.OPT file to build TOOLS
$ IF (SHARED.GT.0)
$   THEN
$	OPEN/WRITE OUT TIFF$SHR.OPT
$	WRITE OUT "[]TIFF/LIB" 
$	WRITE OUT "[-.PORT]LIBPORT/LIB"
$	WRITE OUT JPEG_LIBRARY_PATH
$ 	WRITE OUT "ZLIB:LIBZ/LIB"
$	CLOSE OUT
$ ENDIF
$!
$! Writing OPT.OPT file to build TOOLS
$OPEN/WRITE OUT OPT.OPT
$ IF (SHARED.GT.0)
$   THEN
$   	WRITE OUT "[-.LIBTIFF]TIFF$SHR/SHARE"
$	WRITE OUT JPEG_LIBRARY_PATH
$   ELSE
$   	WRITE OUT "[-.LIBTIFF]TIFF/LIB"
$	WRITE OUT "[-.PORT]LIBPORT/LIB
$	WRITE OUT JPEG_LIBRARY_PATH
$ ENDIF
$ WRITE OUT "ZLIB:LIBZ/LIB"
$CLOSE OUT
$!
$!

$COPY SYS$INPUT [.LIBTIFF]DESCRIP.MMS
# Makefile for DEC C compilers.
#

INCL    = /INCLUDE=(JPEG,ZLIB,[])

CFLAGS =  $(INCL)/OPT=(INLINE=SPEED)

OBJ_SYSDEP_MODULE = tif_vms.obj

OBJ     = tif_aux.obj, \
        tif_close.obj, \
        tif_codec.obj, \
        tif_color.obj, \
        tif_compress.obj, \
        tif_dir.obj, \
        tif_dirinfo.obj, \
        tif_dirread.obj, \
        tif_dirwrite.obj, \
        tif_dumpmode.obj, \
        tif_error.obj, \
        tif_extension.obj, \
        tif_fax3.obj, \
        tif_fax3sm.obj, \
        tif_getimage.obj, \
        tif_jpeg.obj, \
        tif_ojpeg.obj, \
        tif_flush.obj, \
        tif_luv.obj, \
        tif_lzw.obj, \
        tif_next.obj, \
        tif_open.obj, \
        tif_packbits.obj, \
        tif_pixarlog.obj, \
        tif_predict.obj, \
        tif_print.obj, \
        tif_read.obj, \
        tif_stream.obj, \
        tif_swab.obj, \
        tif_strip.obj, \
        tif_thunder.obj, \
        tif_tile.obj, \
        tif_version.obj, \
        tif_warning.obj, \
        tif_write.obj, \
        tif_zip.obj, \
        $(OBJ_SYSDEP_MODULE)
$!
$IF (SHARED.GT.0)
$ THEN
$	APP SYS$INPUT [.LIBTIFF]DESCRIP.MMS
ALL : tiff.olb, tiff$shr.exe
	$WRITE SYS$OUTPUT "Done"

tiff$shr.exe : tiff.olb
	LINK/SHARE=TIFF$SHR.EXE TIF_AUX,[-]TIFF$SHR/OPT, [-]TIFF$DEF/OPT
	COPY TIFF$SHR.EXE SYS$SHARE
	PURGE SYS$SHARE:TIFF$SHR.EXE

$ ELSE
$	APP SYS$INPUT [.LIBTIFF]DESCRIP.MMS
ALL : tiff.olb
	$WRITE SYS$OUTPUT "Done"

$ENDIF
$!
$!
$ APP SYS$INPUT [.LIBTIFF]DESCRIP.MMS

tiff.olb :  tif_config.h, tiffconf.h $(OBJ)
        lib/crea tiff.olb $(OBJ)

tif_config.h : tif_config.h-vms
        copy tif_config.h-vms tif_config.h

tiffconf.h : tiffconf.h-vms
        copy tiffconf.h-vms tiffconf.h

clean :
        del *.obj;*
        del *.olb;*
$!
$!
$! If we've maked shared libtiff, so use shared library to compile TOOLS
$!
$!
$COPY SYS$INPUT [.TOOLS]DESCRIP.MMS
INCL            = /INCL=([],[-.LIBTIFF])
CFLAGS = $(INCL)/OPT=(INLINE=SPEED)

TARGETS =       bmp2tiff.exe tiffcp.exe tiffinfo.exe tiffdump.exe \
                fax2tiff.exe fax2ps.exe gif2tiff.exe pal2rgb.exe ppm2tiff.exe \
                rgb2ycbcr.exe thumbnail.exe ras2tiff.exe raw2tiff.exe \
                tiff2bw.exe tiff2rgba.exe tiff2pdf.exe tiff2ps.exe \
                tiffcmp.exe tiffdither.exe tiffmedian.exe 
#tiffsplit.exe

tiffsplit.exe : $(TARGETS)

bmp2tiff.exe : bmp2tiff.obj
        LINK bmp2tiff, [-]OPT/OPT

tiffcp.exe : tiffcp.obj
        LINK tiffcp, [-]OPT/OPT

tiffinfo.exe : tiffinfo.obj
        LINK tiffinfo, [-]OPT/OPT

tiffdump.exe : tiffdump.obj
        LINK tiffdump, [-]OPT/OPT

fax2tiff.exe : fax2tiff.obj
        LINK fax2tiff, [-]OPT/OPT

fax2ps.exe : fax2ps.obj
        LINK fax2ps, [-]OPT/OPT

gif2tiff.exe : gif2tiff.obj
        LINK gif2tiff, [-]OPT/OPT

pal2rgb.exe : pal2rgb.obj
        LINK pal2rgb, [-]OPT/OPT

ppm2tiff.exe : ppm2tiff.obj
        LINK ppm2tiff, [-]OPT/OPT

rgb2ycbcr.exe : rgb2ycbcr.obj
        LINK rgb2ycbcr, [-]OPT/OPT

thumbnail.exe : thumbnail.obj
        LINK thumbnail, [-]OPT/OPT

ras2tiff.exe : ras2tiff.obj
        LINK ras2tiff, [-]OPT/OPT

raw2tiff.exe : raw2tiff.obj
        LINK raw2tiff, [-]OPT/OPT

tiff2bw.exe : tiff2bw.obj
        LINK tiff2bw, [-]OPT/OPT

tiff2rgba.exe : tiff2rgba.obj
        LINK tiff2rgba, [-]OPT/OPT

tiff2pdf.obj : tiff2pdf.c
        CC/NOWARN $(CFLAGS) tiff2pdf

tiff2pdf.exe : tiff2pdf.obj
        LINK tiff2pdf, [-]OPT/OPT

tiff2ps.exe : tiff2ps.obj
        LINK tiff2ps, [-]OPT/OPT

tiffcmp.exe : tiffcmp.obj
        LINK tiffcmp, [-]OPT/OPT

tiffdither.exe : tiffdither.obj
        LINK tiffdither, [-]OPT/OPT

tiffmedian.exe : tiffmedian.obj
        LINK tiffmedian, [-]OPT/OPT

tiffsplit.exe : tiffsplit.obj
        LINK tiffsplit, [-]OPT/OPT

CLEAN :
	DEL ALL.;*
	DEL *.OBJ;*
	DEL *.EXE;*
$!
$!
$!copiing and patching TIFF_CONF.H, TIF_CONFIG.H
$!
$CURRENT = F$ENVIRONMENT (""DEFAULT"")
$CURRENT[F$LOCATE("]",CURRENT),9]:=".LIBTIFF]"
$WRITE SYS$OUTPUT CURRENT
$COPY 'CURRENT'TIFFCONF.H-VMS 'CURRENT'TIFFCONF.H
$COPY 'CURRENT'TIF_CONFIG.H-VMS 'CURRENT'TIF_CONFIG.H
$PURGE 'CURRENT'TIFFCONF.H
$PURGE 'CURRENT'TIF_CONFIG.H
$OPEN/APPEND OUT 'CURRENT'TIF_CONFIG.H
$IF HAVE_LFIND.EQ.1
$   THEN 
$	WRITE OUT "#define HAVE_SEARCH_H 1" 
$   ELSE
$	WRITE OUT "#undef HAVE_SEARCH_H"
$ENDIF
$CLOSE OUT
$!
$!
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Now you can type @BUILD "
$!
$EXIT:
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$DEL TEST.OBJ;*
$DEL TEST.C;*
$DEL TEST.EXE;*
$DEL TEST.OPT;*
$DEAS SYS$ERROR
$DEAS SYS$OUTPUT
