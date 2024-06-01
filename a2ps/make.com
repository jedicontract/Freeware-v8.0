$ !
$ ! Updated 26-OCT-2004, M. Duffy, to support IA64, Alpha, VAX.
$ ! The previous version of this file was renamed to MAKE.COM_ORIG
$ !
$ ! Look for the compiler used
$ !
$ ccopt = "/def=ansic"
$ !
$ if f$getsyi("arch_name") .eqs. "VAX"
$ then
$  if f$search("SYS$SYSTEM:DECC$COMPILER.EXE").eqs.""
$   then
$   else
$    ccopt = "/decc/prefix=all"+ccopt
$  endif
$ endif
$ !
$ if f$getsyi("arch_name") .eqs. "Alpha"
$ then
$   ccopt = "/prefix=all"+ccopt
$ endif
$ !
$ if f$getsyi("arch_name") .eqs. "IA64"
$ then
$   ! Compile it the same way as on Alpha.  Interesting, the compiler warnings
$   ! are the same on VAX and IA64, with no warnings on Alpha.
$   ccopt = "/prefix=all"+ccopt
$ endif
$ !
$ cc'ccopt a2ps.c
$ link a2ps
$ exit
