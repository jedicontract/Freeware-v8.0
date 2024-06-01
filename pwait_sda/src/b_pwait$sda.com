$!
$! B_PWAIT$SDA.COM - build PWAIT SDA extension
$!
$! COPYRIGHT NOTICE
$!
$! This software is COPYRIGHT (c) 2004, Ian Miller. ALL RIGHTS RESERVED.
$!
$! Released under licence described in aaareadme.txt
$!
$ IF P1 .NES ""
$ THEN
$     CCFLAGS = "/DEBUG/NOOPT/DEFINE=(""DEBUG=1"")"
$     LINKFLAGS = "/DEBUG"
$ ELSE
$     CCFLAGS = "/NOOPT"
$     LINKFLAGS = ""
$ ENDIF
$ IF f$getsyi("ARCH_NAME") .eqs. "IA64" THEN goto IA64_BUILD
$ IF f$getsyi("ARCH_NAME") .eqs. "VAX" THEN goto VAX_BUILD
$ SET COMMAND/OBJECT=PWAIT_CLD.OBJ PWAIT_CLD.CLD
$ CC/OBJECT=sda_utils.obj sda_utils.c+alpha$library:sys$lib_c/lib
$ CC/FLOAT=D_FLOAT'CCFLAGS' pwait$sda.c+alpha$library:sys$lib_c/lib
$ link'LINKFLAGS'/share/sysexe=selective pwait$sda.obj,pwait_cld.obj,sda_utils.obj, -
	alpha$library:vms$volatile_private_interfaces /library, -
  sys$input/option
    symbol_vector=(sda$extend=procedure)
    symbol_vector=(sda$extend_version=data)
$!
$exit
$IA64_BUILD:
$ SET COMMAND/OBJECT=PWAIT_CLD.OBJ PWAIT_CLD.CLD
$ CC/OBJECT=sda_utils.obj sda_utils.c+sys$library:sys$lib_c/lib
$ CC'CCFLAGS'/float=d_float pwait$sda.c+sys$library:sys$lib_c/lib
$ LINK'LINKFLAGS'/share/sysexe=selective pwait$sda.obj,pwait_cld.obj,sda_utils.obj, -
        sys$library:vms$volatile_private_interfaces /library, -
  sys$input/option
    symbol_vector=(sda$extend=procedure)
    symbol_vector=(sda$extend_version=data)
$ EXIT
$VAX_BUILD:
$ WRITE SYS$OUTPUT "PWAIT$SDA does not (yet) work on OpenVMS VAX"
$ EXIT
