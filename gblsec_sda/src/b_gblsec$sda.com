$!
$! B_GBLSEC$SDA.COM - build GBLSEC SDA extension
$!
$! COPYRIGHT NOTICE
$!
$! This software is COPYRIGHT (c) 2005, Ian Miller. ALL RIGHTS RESERVED.
$!
$! Released under licence described in aaareadme.txt
$!
$!
$ IF F$GETSYI("ARCH_NAME") .EQS. "VAX"
$ THEN
$     WRITE SYS$OUTPUT "GBLSEC$SDA is not supported on OpenVMS VAX"
$     EXIT
$ ENDIF
$ IF P1 .NES. ""
$ THEN
$     CCFLAGS = "/DEBUG/NOOPT/DEFINE=(""DEBUG=1"")"
$     LINKFLAGS = "/DEBUG"
$ ELSE
$     CCFLAGS = "/NOOPT"
$     LINKFLAGS = ""
$ ENDIF
$ cc'CCFLAGS'/INSTRUCTION=NOFLOAT gblsec$sda.c+sys$library:sys$lib_c/lib      
$ set command/object=gblsec_cld.obj gblsec_cld.cld
$ message/object=gblsec_msg.obj gblsec_msg.msg
$ link'LINKFLAGS'/share/SYSEXE gblsec$sda.obj, gblsec_cld.obj, gblsec_msg.obj, -
        sys$library:vms$volatile_private_interfaces /library, -
  sys$input/option
    symbol_vector=(sda$extend=procedure)
    symbol_vector=(sda$extend_version=data)
$!
$ EXIT
