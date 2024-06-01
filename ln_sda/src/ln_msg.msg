! LN utility messages
! COPYRIGHT NOTICE
!
! This software is COPYRIGHT (c) 2006, Ian Miller. ALL RIGHTS RESERVED.
! 
! Released user licence in AAAREADME.TXT
!
.FACILITY LN,942/PREFIX=LN__
!	
.SEVERITY SUCCESS
NORMAL <Normal successful return>/FAO=0
!
.SEVERITY INFORMATIONAL
MAXTABLES	<Max number of tables exceeded>
WISHLIST	<Not yet Implemented>
!
.SEVERITY WARNING
WARNING <Warning>/FAO=0
NOCOMMAND       <Nothing on the command line>/FAO=0
NOTFOUND	<Name not found>
TNOTFOUND	<Name table not found>
TOODEEP		<Too many levels of translation for !SL !AF>/FAO=2
GETSDIR	<Failed to get system logical name table directory address>/FAO=0
GETPDIR <Failed to get process logical name table directory address>/FAO=0
GETPHTA	<Failed to get process hash table address>/FAO=0
GETPHT	<Failed to get process hash table>/FAO=0
GETSHTA	<Failed to get system hash table address>/FAO=0
GETSHT	<Failed to get system hash table>/FAO=0
VTL	<Name value too long - index !SL length !UL>/FAO=2
!
.SEVERITY ERROR
SYNTAX <Syntax error>/FAO=0
READSYM <SDA$READ_SYMFILE failed ccode = !XL>/FAO=1
GETVALUE <Failed to get value>/FAO=0
GETVALUEA <Failed to get address of !AZ>/FAO=1
NTL <name is too long>/FAO=0
INVLNMB		<Invalid LNMB found at !8X with type !UB>/FAO=2
INVLNMH		<Invalid LNMH found at !8X with type !UB>/FAO=2
GETCHDR		<Error getting cache block list header>/FAO=0
GETCB		<Error getting cache block at !8XL>/FAO=1
INVCB		<Invalid block type !UL found in cache block at !8XL>/FAO=2
GETTH		<Failed to get table header @ !8XL>/FAO=1
GETLNMB		<Failed to get LNMB @ !XL>/FAO=1
LNTL		<Logical name too long - LNMB @ !XL>/FAO=1
GETV		<Error getting name value @ !XL>/FAO=1
!
.SEVERITY SEVERE
FATAL <Fatal error>/FAO=0
BUG <Internal error !UL>/FAO=1
GETHTA  <Failed to get hash table array address	>/FAO=0
GETHT	<Failed to get hash table array>/FAO=0
!
.END
