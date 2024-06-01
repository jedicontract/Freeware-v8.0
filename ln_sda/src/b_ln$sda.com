$!
$! B_LN$SDA.COM - build LN SDA extension
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
$     WRITE SYS$OUTPUT "LN$SDA is not supported on OpenVMS VAX"
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
$ cc'CCFLAGS'/INSTRUCTION=NOFLOAT LN_sda.c+sys$library:sys$lib_c/lib      
$ set command/object=LN_cld.obj LN_cld.cld
$ message/object=LN_msg.obj LN_msg.msg
$ link'LINKFLAGS'/share=SYS$DISK:[]LN$SDA.EXE/SYSEXE LN_sda.obj, LN_cld.obj, LN_msg.obj, -
        sys$library:vms$volatile_private_interfaces /library, -
  sys$input/option
    symbol_vector=(sda$extend=procedure)
    symbol_vector=(sda$extend_version=data)
$!
$ EXIT
