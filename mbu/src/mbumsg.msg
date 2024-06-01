! MBU utility messages
! COPYRIGHT NOTICE
!
! This software is COPYRIGHT (c) 2004, Ian Miller. ALL RIGHTS RESERVED.
! Permission is granted for not-for-profit redistribution, provided all source
! and object code remain unchanged from the original distribution, and that all
! copyright notices remain intact.
!
! DISCLAIMER
!
! This software is provided "AS IS". The author makes no representations or
! warranties with respect to the software and specifically disclaim any implied
! warranties of merchantability or fitness for any particular purpose.
!
.FACILITY MBU,99/PREFIX=MBU__
!	
.SEVERITY SUCCESS
NORMAL <Normal successful return>/FAO=0
!
.SEVERITY INFORMATIONAL
DEFAULTS <Command defaults are>/FAO=0
MBUCHAR <Characteristics of !AS mailbox !AS>/FAO=2
NOMSG <no messages to display>/FAO=0
VERSION <version !AS built on !AS OpenVMS !AS !AS>/FAO=4
!
.SEVERITY WARNING
WARNING <Warning>/FAO=0
WISHLIST <Not yet implemented>/FAO=0
INVVAL <Invalid value>/FAO=0
!
.SEVERITY ERROR
ERROR <Error>/FAO=0
INVCMD <Command is only valid within a command file>/FAO=0
TOODEEP <Command file nesting too deep>/FAO=0
INVCOM <Command not valid within a command file>/FAO=0
BADFORMAT <Invalid data format>/FAO=0
BADSIZE <Invalid data size>/FAO=0
BADFP <Invalid file pointer>/FAO=0
MAXMSG <Message length exceeds mailbox maximum !UL>/FAO=1
INVINP <Input not valid for format>/FAO=0
SYNTAX <Syntax error>/FAO=0
!
.SEVERITY SEVERE
FATAL <Fatal error>/FAO=0
BUG <Internal error !UL>/FAO=1
!
.END
