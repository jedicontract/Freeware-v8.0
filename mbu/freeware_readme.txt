MBU, UTILITIES, Mailbox utility

  MBU is a utility for VMS mailboxes. MBU can create mailboxes,
  delete mailboxes, read and write mailbox messages, and display and 
  change the characteristics of mailboxes. It can also view messages in a 
  mailbox without reading them. Lots of detail is shown about a mailbox
  including currently waiting requests, number of messages waiting etc.
  The SHOW MAILBOX command allows selection by various criteria including
  is the mailbox empty or not, is the mailbox in use or not and so on.

  There is support for command files and scripts of mailbox messages to be
  repeated.

  This program was written by Ian Miller.

  Bug reports and comments to

	miller@encompasserve.org


  WARNING

  The SET, SHOW and VIEW commands execute some kernel mode code to access
  system data structures. This requires CMKRNL privilege. This code appears
  to work on my system (OpenVMS Alpha V7.3-2) but I can't say if it will work on 
  yours. It may crash your system. If you run without CMKRNL privilege then
  MBU will work except for VIEW and SET MAILBOX commands. (SHOW MAILBOX works
  but displays less information).
    
  MBU has been built on the following versions of OpenVMS Alpha and appears to work
  V6.2, V7.1, V7.2, V7.2-1, v7.3, V7.3-1, V7.3-2.
  MBU has been built on OpenVMS I64 V8.2 and appears to work.

  MBU does not currently build or work on VAX/VMS. I hope to fix this one day 
  - any help appriciated (the problems are in MBU8.MAR)

  CHANGES
  V1.3 	Port to Alpha, display extra information 
  V1.4 	Show if mailbox is permanent or temporary. 
 	Display mailboxes matching specified criteria e.g. not empty.
	Display I/O queues for mailbox.
	Implement SET VERIFY, SHOW VERSION commands.
  V1.5	Display I/O function details.
	Implement /TIMEOUT and /READERCHECK for WRITE.
	Implement /TIMEOUT and /WRITERCHECK for READ.
	Implement SET MAILBOX/PROTECTION.
	Fix some bugs.
  V1.6	Fix SHOW MAILBOX on VMS V8.2
	Fix bugs in SET MAILBOX
	Use LIB$TABLE_PARSE instead of LIB$TPARSE on Alpha and I64
	Port to VMS I64.

  TO BUILD MBU

  Either use MMK/MMS and MBU.MMS 

	MMK/DESCRIP=MBU.MMS
  
  (add /MACRO=DEBUG=1 to build a debug version)

  or use the DCL procedure B_MBU.COM

  	@B_MBU.COM

  (add a parameter of DEBUG to build a debug version)

  There is a MBU.HLP for insertion into SYS$LIBRARY:HELPLIB.HLB or other
  help library.


  INSTALLATION

  Build and copy MBU.EXE (or one of the pre-built .EXE_Vxx) to some
  known location. Insert the help file in a help library. 
  Optionally define a symbol MBU::== $dev:[dir]MBU.EXE 

  COPYRIGHT NOTICE

  This software is COPYRIGHT © 2004, Ian Miller. ALL RIGHTS RESERVED.
  Permission is granted for not-for-profit redistribution, provided all source
  and object code remain unchanged from the original distribution, and that all
  copyright notices remain intact.

  DISCLAIMER

  This software is provided "AS IS". The author makes no representations or
  warranties with respect to the software and specifically disclaim any implied
  warranties of merchantability or fitness for any particular purpose.
