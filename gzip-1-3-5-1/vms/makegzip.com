$! MAKEGZIP.COM -- build procedure for OpenVMS V7.3 and later
$!
$! updates for:
$!   OpenVMS Freeware V8.0
$!   Stephen Hoffman
$!   HP OpenVMS Engineering
$!   29-Jun-2006
$!
$! Various changes from the stock 1.3.5-1 makegzip.com are
$! included here, including the following:
$!
$!   o this version uses subdirectories for all
$!     architecture-specific files and for all
$!     files generated here.  This both for easier
$!     clean-up, and it allows one directory tree
$!     to host all OpenVMS architectures.
$!
$!   o the OpenVMS I64 cross-build mechanisms have
$!     been removed; all architectures now use only
$!     the native builds.
$!
$!   o The current architecture name is retrieved
$!     directly from f$getsyi.
$!
$!   o command procedure output has been updated.
$!
$!   o added source files apparently new to gzip
$!     circa 1.3.5-1 into this procedure.
$!
$!   o removed null crypt.c; suppresses a warning
$!     for a null object.
$!
$!   o provided config.h, tweaked tailor.h; the
$!     OpenVMS versions of both are now in [.VMS].
$!
$!   o add config.h onto getopt.c and getopt1.c
$!     compilations to suppress errors.
$!
$!
$
$ On Control_Y Then Goto The_Error
$ On Error Then Goto The_Error
$
$
$ arch = "_" + f$getsyi("ARCH_NAME")
$
$
$ if f$search("vms''arch'.dir") .eqs. ""
$ then
$   create/directory [.vms'arch']
$ else
$   if f$search("[.vms''arch']*.*.*") .nes. "" then delete [.vms'arch']*.*.* !/conf
$ endif
$ Libr/Object/Create [.vms'arch']Gzip.Olb
$
$ call DoCompile gzip.c
$ call DoCompile zip.c
$ call DoCompile deflate.c
$ call DoCompile trees.c
$ call DoCompile bits.c
$ call DoCompile unzip.c 
$ call DoCompile inflate.c 
$ call DoCompile util.c 
$!$ call DoCompile crypt.c 
$ call DoCompile lzw.c 
$ call DoCompile unlzw.c 
$ call DoCompile unpack.c
$ call DoCompile unlzh.c 
$ call DoCompile rpmatch.c
$ call DoCompile yesno.c
$ call DoCompile getopt.c /prefix=except=(getopt,optarg,optind,opterr,optopt)/first_inc=[.vms]config.h
$ call DoCompile getopt1.c /prefix=except=(getopt,optarg,optind,opterr,optopt)/first_inc=[.vms]config.h
$ call DoCompile [.vms]vms.c
$
$The_Link:
$
$ write sys$output "$ link /exec=[.vms''arch']gzip.exe -"
$ write sys$output "  [.vms''arch']gzip.olb/libr/include=gzip, -"
$ write sys$output "  [.vms''arch']gzip.olb/librar"
$
$ link /exec=[.vms'arch']gzip.exe -
  [.vms'arch']gzip.olb/libr/include=gzip, -
  [.vms'arch']gzip.olb/librar
$
$ write sys$output "Done."
$
$The_Exit:
$ Save_Status = $STATUS
$ exit Save_Status
$
$The_Error:
$
$ if f$search("[.vms''arch']*.*.*") .nes. "" then delete [.vms'arch']*.*.* !/conf
$ goto The_Exit
$
$DoCompile: Subroutine
$
$
$ arch = "_" + f$getsyi("ARCH_NAME")
$ obj = f$parse("[.vms''arch'].OBJ",p1)
$ lis = f$parse("[.vms''arch'].LIS",p1)
$
$ if f$search(lis) .nes. "" then delete 'lis' !/confirm
$ if f$search(obj) .nes. "" then delete 'obj' !/confirm
$
$ write sys$output "$ cc/decc/object=''obj' -"
$ write sys$output "  /include=([.vms],[.vms''arch'],sys$disk:[]) -"
$ if f$length(p2) .ne. 0
$ then
$   write sys$output "  ''p1' -"
$   write sys$output "  ''p2'
$ else
$   write sys$output "  ''p1'"
$ endif
$
$ cc/decc/object='obj' -
  /include=([.vms],[.vms'arch'],sys$disk:[]) -
  'p1' 'p2'
$
$ Libr/Object/Insert [.vms'arch']Gzip.Olb 'obj'
$
$ endsubroutine
$ Exit
