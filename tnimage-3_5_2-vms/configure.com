$! Configure procedure 
$! (c) Alexey Chupahin  18-APR-2006
$! 
$!
$!
$! HAVE_JPEG,  HAVE_JPEG_SHARED   -- test for libjpeg  JPEG_LIBRARY_PATH - a path of library to be used
$! HAVE_ZLIB,  HAVE_ZLIB_SHARED  -- test for libz
$! HAVE_LFIND, HAVE_STRCASECMP   -- correspond libraries
$! SHARED=64 - creating shared libtiff for Alpha and (may be IA64)
$! SHARED=32 - creating shared libtiff for VAX 
$! 
$!
$ SET NOON
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Configuring TNIMAGE "
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
$!"Checking for correct zlib library    "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST_CHAPG.OBJ/INCLUDE=(ZLIB) SYS$INPUT
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
$       GOTO EXIT
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST_CHAPG TEST_CHAPG,ZLIB:LIBZ/LIB
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_ZLIB=0
$       GOTO EXIT
$  ELSE
$       HAVE_ZLIB=1
$ENDIF
$IF (HAVE_ZLIB.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for correct zlib library ...   Yes"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for correct zlib library ...   No"
$       WRITE SYS$OUTPUT "This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$ENDIF
$RUN TEST_CHAPG
$!
$! Checking for JPEG ...
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ DEL TEST_CHAPG.*;*
$ CC/OBJECT=TEST_CHAPG.OBJ/INCLUDE=(JPEG) SYS$INPUT
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
$       WRITE SYS$OUTPUT "Checking for jpeg library ...   No"
$	HAVE_JPEG=0
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST_CHAPG TEST_CHAPG,JPEG:LIBJPEG/OPT
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
$       WRITE SYS$OUTPUT "Checking for jpeg library ...   Yes"
$	JPEG_LIBRARY_PATH="JPEG:LIBJPEG/OPT"
$	RUN TEST_CHAPG
$  ELSE
$       WRITE SYS$OUTPUT "Checking for jpeg library ...   No"
$ENDIF
$!
$!
$ IF ( (HAVE_JPEG.EQ.0) )
$    THEN
$       WRITE SYS$OUTPUT "No JPEG library installed. This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$	GOTO EXIT
$ ENDIF
$!
$!
$!
$! Checking for PNG ...
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ DEL TEST_CHAPG.*;*
$ CC/OBJECT=TEST_CHAPG.OBJ/INCLUDE=(PNG,ZLIB) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <png.h>
int
main(int argc, char *argv[])
{
   int multiple = 0;
   int ierror = 0;

   
   printf("libpng version %s\n", PNG_LIBPNG_VER_STRING);
}

$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       WRITE SYS$OUTPUT "Checking for PNG library ...   No"
$       HAVE_PNG=0
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$TMP = $STATUS
$DEAS  SYS$OUTPUT
$DEAS  SYS$ERROR
$       WRITE SYS$OUTPUT "Checking for PNG library ...   No"
$       HAVE_PNG=0
$ENDIF
$DEFINE SYS$OUTPUT _NLA0:
$DEFINE SYS$ERROR _NLA0:
LIN TEST_CHAPG,PNG:LIBPNG/LIB
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_PNG=0
$  ELSE
$       HAVE_PNG=1
$ENDIF
$IF (HAVE_PNG.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for PNG library ...   Yes"
$       PNG_LIBRARY_PATH="PNG:LIBPNG/LIB"
$       RUN TEST_CHAPG
$  ELSE
$       WRITE SYS$OUTPUT "Checking for PNG library ...   No"
$       WRITE SYS$OUTPUT "No PNG library installed. This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$       GOTO EXIT
$ ENDIF
$!
$!
$! Checking for libTIFF ...
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ DEL TEST_CHAPG.*;*
$ CC/OBJECT=TEST_CHAPG.OBJ/INCLUDE=(TIFF) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <tiffio.h>
   int main()
     {
	printf("checking libTIFF version:\t%s\n\n", TIFFGetVersion());
        return 0;
     }
$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       WRITE SYS$OUTPUT "Checking for TIFF library ...   No"
$       HAVE_TIFF=0
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK TEST_CHAPG, TIFF:LIBTIFF/OPT, ZLIB:LIBZ/LIB
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_TIFF=0
$  ELSE
$       HAVE_TIFF=1
$ENDIF
$IF (HAVE_TIFF.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for TIFF library ...   Yes"
$       TIFF_LIBRARY_PATH="TIFF:LIBTIFF/OPT"
$       RUN TEST_CHAPG
$  ELSE
$       WRITE SYS$OUTPUT "Checking for TIFF library ...   No"
$ENDIF
$!
$!
$ IF ( (HAVE_TIFF.EQ.0) )
$    THEN
$       WRITE SYS$OUTPUT "No JPEG library installed. This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$       GOTO EXIT
$ ENDIF
$!
$!

$!
$!
$! Checking fo LEXLIB
$!
$
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ DEL TEST_CHAPG.*;*
$ CC/OBJECT=TEST_CHAPG.OBJ/INCLUDE=(LEX) SYS$INPUT
char yywrap ();
int
main ()
{
yywrap ();
  ;
  return 0;
}

$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$!
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10B90001)
$  THEN
$       HAVE_LEX=0
$ENDIF
$!
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ LINK TEST_CHAPG,LEX:FLEXLIB/LIB
$TMP = $STATUS
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_LEX=0
$  ELSE
$	HAVE_LEX=1
$ENDIF
$!
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT


$IF (HAVE_LEX.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for FLEXLIB library ...   Yes"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for FLEXLIB library ...   No"
$ENDIF
$!
$ IF ( (HAVE_LEX.EQ.0) )
$    THEN
$       WRITE SYS$OUTPUT "No FLEXLIB library installed. This is fatal. Please download and install from HP OpenVMS Freeware CDROM. Define  LEX to the path, containing FLEXLIB.OLB. "
$	WRITE "Example: $DEFINE LEX DKA100:[MY_DIRECORY.FLEX] "
$       GOTO EXIT
$ ENDIF



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
$ WRITE OUT "$",MAKE
$CLOSE OUT
$!
$ WRITE SYS$OUTPUT "Writing TNIMAGE.OPT"
$OPEN/WRITE OUT TNIMAGE.OPT 
$ WRITE OUT "Sys$Share:DECW$XMLibShr12.exe /Share"
$ WRITE OUT "SYS$SHARE:DECW$XMuLibShrR5.EXE/SHARE"
$ WRITE OUT "SYS$SHARE:DECW$XExtLibShr.EXE/SHARE"
$ WRITE OUT "Sys$Share:DECW$XTLibShrR5.exe /Share"
$ WRITE OUT "Sys$Share:DECW$XLibShr.exe /Share"
$CLOSE OUT
$!
$!
$WRITE SYS$OUTPUT "Creating DESCRIP.MMS"
$COPY SYS$INPUT DESCRIP.MMS
# Makefile for DEC C compilers.
#

INCL= /INCLUDE=([], JPEG, PNG, TIFF, X11, ZLIB)

CFLAGS = $(INCL) $(DEFS)
CXXFLAGS = $(INCL) $(DEFS) /NOWARN

OBJ1 = \
xmtnimage1.obj,\
xmtnimage10.obj,\
xmtnimage11.obj,\
xmtnimage12.obj,\
xmtnimage13.obj,\
xmtnimage14.obj,\
xmtnimage15.obj,\
xmtnimage16.obj,\
xmtnimage17.obj,\
xmtnimage18.obj,\
xmtnimage19.obj,\
xmtnimage2.obj,\
xmtnimage20.obj,\
xmtnimage21.obj,\
xmtnimage22.obj,\
xmtnimage23.obj,\
xmtnimage24.obj,\
xmtnimage25.obj

OBJ2=\
xmtnimage26.obj,\
xmtnimage27.obj,\
xmtnimage28.obj,\
xmtnimage29.obj,\
xmtnimage3.obj,\
xmtnimage30.obj,\
xmtnimage31.obj,\
xmtnimage32.obj,\
xmtnimage33.obj,\
xmtnimage34.obj,\
xmtnimage35.obj,\
xmtnimage36.obj,\
xmtnimage37.obj,\
xmtnimage38.obj,\
xmtnimage39.obj,\
xmtnimage4.obj,\
xmtnimage40.obj,\
xmtnimage41.obj,\
xmtnimage42.obj,\
xmtnimage43.obj,\
xmtnimage44.obj,\
xmtnimage45.obj,\
xmtnimage46.obj,\
xmtnimage47.obj,\
xmtnimage48.obj,\
xmtnimage49.obj

OBJ3 = \
xmtnimage5.obj,\
xmtnimage50.obj,\
xmtnimage51.obj,\
xmtnimage52.obj,\
xmtnimage53.obj,\
xmtnimage54.obj,\
xmtnimage55.obj,\
xmtnimage56.obj,\
xmtnimage57.obj,\
xmtnimage58.obj,\
xmtnimage59.obj,\
xmtnimage6.obj,\
xmtnimage60.obj,\
xmtnimage61.obj,\
xmtnimage62.obj,\
xmtnimage63.obj,\
xmtnimage64.obj,\
xmtnimage65.obj,\
xmtnimage66.obj,\
xmtnimage67.obj,\
xmtnimage68.obj,\
xmtnimage7.obj,\
xmtnimage8.obj,\
xmtnimage9.obj,\
y.obj,\
lex.obj,\
vms-patch.obj


OBJ = $(OBJ1),$(OBJ2),$(OBJ3)

#*.obj.CC : $(OBJ)
#        CXX $(CFLAGS) $<

ALL : TNIMAGE.EXE, TNIMAGE.OLB, PLUGIN.EXE, READTIF.EXE
        $WRITE SYS$OUTPUT "Build process done"

Y.OBJ : Y.TAB_C
        CXX $(CXXFLAGS) Y.TAB_C

LEX.OBJ : LEX.YY_CC
        CXX $(CXXFLAGS) LEX.YY_CC

PLUGIN.EXE : PLUGIN.OBJ
        CXXLNK PLUGIN,TNIMAGE/LIB, TIFF:LIBTIFF/OPT

# READHDF.CXX,\

READTIF.EXE : READTIF.OBJ
        CXXLNK READTIF, TNIMAGE/LIB, TIFF:LIBTIFF/OPT


TNIMAGE.EXE : XMTNIMAGE.OBJ, TNIMAGE.OLB
        CXXLNK/EXE=TNIMAGE XMTNIMAGE, TNIMAGE/LIB, PNG:LIBPNG/LIB, TIFF:LIBTIFF/OPT, []TNIMAGE/OPT, LEX:flexlib/lib, ZLIB:LIBZ/LIB


TNIMAGE.OLB : $(OBJ)
        lib/crea []TNIMAGE.OLB $(OBJ1)
        lib/ins []TNIMAGE.OLB $(OBJ2)
        lib/ins []TNIMAGE.OLB $(OBJ3)

CLEAN :
	DEL *.OBJ;*
	DEL *.EXE;*


$!
$!
$!

$WRITE SYS$OUTPUT "Creating CONFIG.H"
COPY SYS$INPUT CONFIG.H
/* config.h.  Generated by configure.  */
/* This file is used by configure to create a config.h file. */
/* configure will comment out the undef's if it finds it.    */
/* Most of these variables are never actually used.          */

#ifndef __CONFIG_H
#define __CONFIG_H
#endif



/* Define if you have motif version 1 or higher.  */
#define HAVE_MOTIF 1

/* Define if you have motif version 2 or higher.  */
/* #undef MOTIF2 */

/* Define if you have Xbae.  */
/* #undef HAVE_XBAE */

/* Define if you have libjpeg.  */
#define HAVE_JPEG 1

/* Define if you have the Xpm library (-lXpm).  */
#undef HAVE_XPM

/* Define if you have fcvt.  */
/* #undef HAVE_FCVT */

/* Define if you have gcvt.  */
/* #undef HAVE_GCVT */

/* Define if you have lex.  */
/* #undef HAVE_LEX */

/* Define if you have the cbrt function.  */
#define HAVE_CBRT 1

/* Define if you have the acosh function.  */
#define HAVE_ACOSH 1

/* Define if you have the asinh function.  */
#define HAVE_ASINH 1

/* Define if you have the atanh function.  */
#define HAVE_ATANH 1

/* Define if you have the isinf function.  */
/* #undef HAVE_ISINF */

/* Define if you have the isnan function.  */
#define HAVE_ISNAN 1

/* Define if you have the erf function.  */
#define HAVE_ERF 1

/* Define your OS.  */
/* #undef LINUX */
/* #undef SOLARIS */
/* #undef IRIX */
/* #undef CONVEX */
/* #undef MIPS */
/* #undef DIGITAL */

#ifndef VMS
#       define VMS
#endif

/* Define if you have bytesex.h  */
/* #undef HAVE_BYTESEX_H */

/* Define if you have endian.h  */
/* #undef HAVE_ENDIAN_H */

/* Define if you have Xft.h  */
/* #undef HAVE_XFT */

int socketpair(int domain, int type, int protocol, int fd[2]);
int fork(void);
int mkfifo(const char *path, unsigned short mode);
char *vms2unix(char* path);
#define LITTLE_ENDIAN 1
#define _LITTLE_ENDIAN 1
#define DIGITAL

$!
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$WRITE SYS$OUTPUT "renaming *.CC to *.CXX source files"
$REN *.CC *.CXX
$DEAS SYS$ERROR
$DEAS SYS$OUTPUT
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Now you can type @BUILD "
$!
$EXIT:
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$DEL TEST_CHAPG.*;*
$DEAS SYS$ERROR
$DEAS SYS$OUTPUT
