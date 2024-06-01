! PF utility messages
! COPYRIGHT NOTICE
!
! This software is COPYRIGHT (c) 2006, Ian Miller. ALL RIGHTS RESERVED.
! 
! Released under licence in AAAREADME.TXT
!
.FACILITY PF,995/PREFIX=PF__
!	
.SEVERITY SUCCESS
NORMAL 		<Normal successful return>/FAO=0
!
.SEVERITY INFORMATIONAL
WISHLIST 	<Not yet implemented>/FAO=0
NOTINUSE	<There are no pages in use in this file>/FAO=0
LOOKING		<Searching for proceses using this file>/FAO=0
USERS		<!UL process!1%C!%Ees!%F found to be using this file>/FAO=1
INFO		<File index !UL>/FAO=1
PFREFS		<Total page file references !UQ>/FAO=1
PFTOTALP	<Total pages used by processes !UQ>/FAO=1
PFTOTAL		<Total pages used !UQ>/FAO=1
TOTALSWAP	<Total Size of all Swap Files !UQ pages>/FAO=1
TOTALPAGE	<Total Size of all Page Files !UQ pages>/FAO=1
TOTALREFS	<Total Committed Page File Usage !UQ pages>/FAO=1
TOTALALLO	<Total Allocated Page File !UQ pages>/FAO=1
PINUSE		<!AZ% pagefile is in use>/FAO=1
GBLSEARCH	<Searching for global pages...>/FAO=0
GBLPAGES	<!UQ global pages found>/FAO=1
!               
.SEVERITY WARNING
WARNING 	<Warning>/FAO=0
NOTFOUND	<File index not found in the specified process region>/FAO=0
NOCOMMAND	<Nothing on the command line>/FAO=0
!
.SEVERITY ERROR
SYNTAX 		<Syntax error>/FAO=0
GETVALUE 	<Failed to get value>/FAO=0
GETVALUEA 	<Failed to get address of !AZ>/FAO=1
GETPCBVEC 	<Failed to get PCB vector>/FAO=0
GETSYSPHD 	<Failed to read system PHD>/FAO=0
GETSPTE 	<Failed to get sizeof a PTE>/FAO=0
GETPPT 		<Failed to get process page table>/FAO=0
NOTINS 		<Specified file is not installed>/FAO=0
GETPFN 		<Error reading PFN database entry for PFN !UL>/FAO=1
INVPFI 		<Invalid page file index>/FAO=0
GETPSVECA 	<Error reading page and swap file vector address>/FAO=0
GETPSVEC 	<Error reading page and swap file vector>/FAO=0
INVPSVEC 	<Invalid page and swap file vector>/FAO=0
GETPFL 		<Error reading page file control block>/FAO=0
GETNULPFL 	<Error reading null page file control block address>/FAO=0
INVPFL 		<Invalid page file control block at !UL>/FAO=1
!
.SEVERITY SEVERE
FATAL 		<Fatal error>/FAO=0
READSYM 	<SDA$READ_SYMFILE failed ccode = !XL>/FAO=1
BUG 		<Internal error !UL>/FAO=1
!
.END
