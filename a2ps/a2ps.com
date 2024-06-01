$!-----------------------------------------------------------------------
$!
$! Name      : A2PS.COM
$!
$! Purpose   : convert ASCII file to PostScript and print
$!
$! Arguments : infile, device, orientation
$!
$!
$!---------------------------------------------------------------------
$ ON ERROR     Then Goto EXIT
$ ON CONTROL_Y Then Goto EXIT
$ ascii_to_ps = "$sat1$dka500:[user.ai22.c.a2ps]a2ps.exe"	!LOCAL
$ !
$ verify = f$verify(0)
$ set noverify
$ say := write sys$output
$ say " "
$ say " A2PS    : file [device] [orientation] 
$ say " defaults:      CPS_PS   portrait     
$ say " "
$ !
$ !   input file name
$ !
$ if p1 .eqs. ""  then inquire p1 "input file  "
$ assign/nolog 'p1' infile
$ !
$ ! check if input file exists
$ !
$ infile := 'f$trnlnm("infile")'
$ exist = f$search("''infile'")
$ if exist .eqs. "" then goto nofile
$ !
$ ! construct a unique file name
$ !
$ time = f$time()
$ hour = f$extract(12,2,time)
$ min =  f$extract(15,2,time)
$ sec =  f$extract(18,2,time)
$ file = "plt''hour'''min'''sec'.out"
$ !
$ if p2 .nes. "" .and. p2 .nes. "?" .and. p2 .nes. "CPS_PS" -
  	then goto nodevice 
$ !
$ if p2 .eqs. "" .or. p2 .eqs. "?" 
$ then
$   p2 = "CPS_PS"
$ endif
$ say " "
$ say "Print file: ''infile'"
$ assign/nolog 'file' outfile
$ if f$extract(0,1,''p3') .eqs. "L" 
$ then 
$   orient = ""
$   say "Orientation: landscape"
$ else
$   orient = "-p"
$   say "Orientation: portrait"
$ endif
$ do_it:
$ say " "
$ assign/nolog outfile sys$output
$ ascii_to_ps -nP  -nv "''orient'"  'infile'
$ deassign sys$output
$ deassign infile
$ deassign outfile
$! open/append xx 'file'
$! write xx ""
$! close xx
$ if p2 .eqs. "CPS_PS"
$ then
$   say "Submit to CPS_PS "
$   say "print/que=cps_ps/delete/form=long/nofeed 'file'"
$   goto exit
$ endif
$ say " "
$ !
$ goto exit
$ !
$ ! abnormal exits
$ !
$ nofile:
$ say " "
$ say "File [1m''infile'[0m does not exist"
$ say " "
$ goto exit
$ nodevice:
$ say " "
$ say "no such device: [1m''p2'[0m"
$ say " "
$ !
$ exit:
$ say " "
$ !
$ if verify then set verify
$ exit
$ !
