!
! Make file for MBU
!
! COPYRIGHT NOTICE
!
! This software is COPYRIGHT (c) 2004, Ian Miller. ALL RIGHTS RESERVED.
! Permission is granted for not-for-profit redistribution, provided all source
! and object code remain unchanged from the original distribution, and that all
! copyright notices remain intact.
!
! DISCLAIMER
!
! This software is provided "AS IS". The author makes no representations or
! warranties with respect to the software and specifically disclaim any implied
! warranties of merchantability or fitness for any particular purpose.
!
.IFDEF DEBUG
CFLAGS = /DEBUG/NOOPT
LINKFLAGS = /DEBUG/MAP
AFLAGS = /DEBUG
.ENDIF

.IFDEF __VAX__
OPTFILE = ,MBU.OPT/OPTIONS
SYSLIB = 
.ELSE
SYSEXE = /SYSEXE
syslib = +SYS$LIBRARY:SYS$LIB_C.TLB/LIBR
OPTFILE = 
.ENDIF

OBJS=mbu.obj,mbu1.obj,mbu2.obj,mbu3.obj,mbu4.obj,mbu5.obj,mbu6.obj,mbu7.obj,mbu8.obj,mbucld.obj,mbumsg.obj
mbu.exe : $(OBJS) mbu.opt
	$(LINK)$(SYSEXE) $(LINKFLAGS) $(OBJS) $(OPTFILE)
mbu.obj : mbu.c mbu.h
mbu1.obj : mbu1.c mbu.h
mbu2.obj : mbu2.c mbu.h
mbu3.obj : mbu3.c mbu.h
mbu4.obj : mbu4.c mbu.h
mbu5.obj : mbu5.c mbu.h
	$(CC) $(CFLAGS)/NOOPT MBU5.C $(SYSLIB)
mbu6.obj : mbu6.c mbu.h
mbu7.obj : mbu7.mar
	macro/object=mbu7.obj$(AFLAGS) sys$library:arch_defs.mar+sys$disk:[]mbu7.mar
mbu8.obj : mbu8.mar
	macro/object=mbu8.obj$(AFLAGS)/LIST=mbu8.lis/MACHINE sys$library:arch_defs.mar+sys$disk:[]mbu8.mar
mbucld.obj : mbucld.cld
mbumsg.obj : mbumsg.msg
