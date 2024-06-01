$!
$! B_PF_SDA.COM - build PF SDA extension
$!
$! COPYRIGHT NOTICE
$!
$! This software is COPYRIGHT (c) 2006, Ian Miller. ALL RIGHTS RESERVED.
$!
$! Released under licence described in aaareadme.txt
$!
$!
$ IF F$GETSYI("ARCH_NAME") .EQS. "VAX"
$ THEN
$     WRITE SYS$OUTPUT "PF_SDA is not supported on OpenVMS VAX"
$     EXIT
$ ENDIF
$ IF P1 .NES. ""
$ THEN
$     CCFLAGS = "/DEBUG/NOOPT/DEFINE=(""DEBUG=1"")"
$     LINKFLAGS = "/DEBUG"
$ ELSE
$     CCFLAGS = ""
$     LINKFLAGS = ""
$ ENDIF
$ cc'CCFLAGS' PF_SDA.c+sys$library:sys$lib_c/lib      
$ set command/object=PF_cld.obj PF_cld.cld
$ message/object=PF_msg.obj PF_msg.msg
$ link'LINKFLAGS'/share=PF$SDA.EXE/SYSEXE PF_SDA.obj, PF_cld.obj, PF_msg.obj, -
        sys$library:vms$volatile_private_interfaces /library, -
  sys$input/option
    symbol_vector=(sda$extend=procedure)
    symbol_vector=(sda$extend_version=data)
$!
$ EXIT
