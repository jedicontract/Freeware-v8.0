! GBLSEC utility messages
! COPYRIGHT NOTICE
!
! This software is COPYRIGHT (c) 2005, Ian Miller. ALL RIGHTS RESERVED.
! 
! Released user licence in AAAREADME.TXT
!
.FACILITY GBLSEC,96/PREFIX=GBLSEC__
!	
.SEVERITY SUCCESS
NORMAL <Normal successful return>/FAO=0
!
.SEVERITY INFORMATIONAL
SEARCHING <searching for processes mapped to this global section>/fao=0
!
.SEVERITY WARNING
WARNING <Warning>/FAO=0
NOTFOUND        <File index not found in the specified process region>/FAO=0
NOCOMMAND       <Nothing on the command line>/FAO=0
!
.SEVERITY ERROR
SYNTAX <Syntax error>/FAO=0
NTL <name is too long>/FAO=0
NTS <name is too short>/FAO=0
GNF <Global Section not found>/FAO=0
GSDERR <error !XL getting GSD listhead addr>/FAO=1
READSYM <SDA$READ_SYMFILE failed ccode = !XL>/FAO=1
GETPCBVEC <Failed to get PCB vector>/FAO=0
GETSYSPHD <Failed to read system PHD>/FAO=0
GETGSTE <Failed to read GSTE>/FAO=0
GETGPTE <Failed to get GPTES>/FAO=0
GETVALUE <Failed to get value>/FAO=0
GETVALUEA <Failed to get address of !AZ>/FAO=1
GETSPTE <Failed to get size of a PTE>/FAO=0
GETPPT <Failed to get process page table>/FAO=0
!
.SEVERITY SEVERE
FATAL <Fatal error>/FAO=0
BUG <Internal error !UL>/FAO=1
!
.END
