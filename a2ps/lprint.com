$!-----------------------------------------------------------------------
$!
$! Name      : LPRINT.COM
$!
$! Purpose   : print file, if necessary, convert file to PostScript
$!
$! Arguments : infile, device, orientation
$!
$! Created  12-AUG-1993   Otmar Stahl
$!
$!---------------------------------------------------------------------
$ ON ERROR     Then Goto EXIT
$ ON CONTROL_Y Then Goto EXIT
$ !
$! verify = f$verify(0)
$! set noverify
$ say := write sys$output
$ !
$ ! check if input file exists
$ !
$ assign/nolog 'p1' infile
$ infile := 'f$trnlnm("infile")'
$ deassign infile
$ exist = f$search("''infile'")
$ if exist .eqs. "" then goto nofile
$ if p2 .eqs. "" then p2 := "PRINTER"
$ xy = f$extract(0,5,"''p2'")
$ !
$ if "''xy'" .eqs. "LASER"
$ then
$   open/read ps 'infile'
$   read ps line
$   close ps
$   xx = f$extract(0,2,line)
$   if "''xx'" .nes. "%!"
$   then
$     say "Convert to PS, send file to ''p2'"
$     @sys$manager:a2ps 'infile' 'p2' 'p3' 
$     goto exit
$   else
$     if "''p2'" .eqs. "LASERC"
$     then
$        @sys$manager:titanpr1 'file'
$        goto exit
$     else
$        print/que="''p2'"/nofeed 'infile'
$        goto exit
$     endif
$   endif
$ else
$   if "''p2'" .eqs. "PRINTER"
$   then
$     print 'infile'
$     goto exit
$   else
$     say "Send file to ''p2'"
$     print/que="''p2'" 'infile'
$     goto exit
$   endif
$ endif
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
$ exit:
$ !
$! if verify then set verify
$ exit
$ !
