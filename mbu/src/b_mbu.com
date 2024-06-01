$!
$! Build command file for MBU
$!
$! COPYRIGHT NOTICE
$!
$! This software is COPYRIGHT (c) 2004, Ian Miller. ALL RIGHTS RESERVED.
$! Permission is granted for not-for-profit redistribution, provided all source
$! and object code remain unchanged from the original distribution, and that all
$! copyright notices remain intact.
$!
$! DISCLAIMER
$!
$! This software is provided "AS IS". The author makes no representations or
$! warranties with respect to the software and specifically disclaim any implied
$! warranties of merchantability or fitness for any particular purpose.
$!
$! 
$!
$ ON WARNING THEN EXIT
$ map = "/nomap"
$ list = "/nolist"
$ ccdebug = "/warn=DIS=(GLOBALEXT,ADDRCONSTEXT,NEEDCONSTEXT)"
$ mmdebug = ""
$ lkdebug = ""
$ linkonly = 0
$ arch_type = F$GETSYI("ARCH_TYPE")
$ IF P1 .EQS. "" THEN GOTO L2
$ X=0
$L1:
$ P = F$ELEMENT(X,",",P1)
$ IF P .EQS. "," THEN GOTO L2
$ IF P .EQS. "LINK" THEN linkonly=1
$ IF P .EQS. "DEBUG"
$ THEN
$     ccdebug="/noopt/debug"
$     lkdebug="/debug"
$     mmdebug="/debug"
$ ENDIF
$ IF P .EQS. "MAP" THEN map = "/MAP"
$ IF P .EQS. "LIST" THEN list = "/LIST"
$ X = X + 1
$ GOTO L1
$L2:
$ compile = "CC"+ccdebug+list
$ assemble = "MACRO"+mmdebug
$ IF arch_type .EQ. 1
$ THEN 
$     WRITE SYS$OUTPUT "MBU does not current work on VMS VAX"
$     EXIT
$     IF F$SEARCH("SYS$SYSTEM:DECC$COMPILER.EXE") .EQS. ""
$     THEN
$         WRITE SYS$OUTPUT "No DECC compiler found - assemble and link only
$         GOTO domac
$     ENDIF
$ ENDIF
$ IF arch_type .EQ. 2 .OR. arch_type .EQ. 3
$ THEN
$     IF F$SEARCH("SYS$SYSTEM:DECC$COMPILER.EXE") .EQS. ""
$     THEN
$         WRITE SYS$OUTPUT "No DECC compiler found - assemble and link only
$         GOTO domac
$     ENDIF
$ ENDIF
$ IF linkonly THEN GOTO DOLINK
$!
$ compile MBU.C
$ compile MBU1.C
$ compile MBU2.C
$ compile MBU3.C
$ compile MBU4.C
$ ! if compilied /opt then SHOW MAIL crashes
$ IF arch_type .EQ. 1
$ THEN
$     compile/NOOPT MBU5.C
$ ELSE
$     compile/NOOPT MBU5.C + SYS$LIBRARY:SYS$LIB_C.TLB/LIBRARY
$ ENDIF
$ compile MBU6.C
$DOMAC:
$ assemble/object=mbu7.obj/lis=mbu7.lis SYS$LIBRARY:ARCH_DEFS.MAR+SYS$DISK:[]MBU7.MAR
$ assemble/object=mbu8.obj/lis=mbu8.lis SYS$LIBRARY:ARCH_DEFS.MAR+SYS$DISK:[]MBU8.MAR
$ SET COMMAND/OBJECT MBUCLD.CLD
$ MESSAGE MBUMSG.MSG
$DOLINK:
$ LNK = "LINK"+lkdebug+map
$ IF arch_type .EQ. 1
$ THEN
$     LNK MBU.OBJ,MBU1.OBJ,MBU2,MBU3,MBU4,MBU5,MBU6,MBU7,MBU8,-
    MBUCLD.OBJ,MBUMSG.OBJ,SYS$INPUT/OPTIONS
    SYS$SHARE:VAXCRTL.EXE/SHARE
$ ENDIF
$ IF arch_type .EQ. 2 .OR. arch_type .EQ. 3
$ THEN
$     LNK /SYSEXE MBU.OBJ,MBU1.OBJ,MBU2,MBU3,MBU4,MBU5,MBU6,MBU7,MBU8,-
    MBUCLD.OBJ,MBUMSG.OBJ
$ ENDIF
$ EXIT
