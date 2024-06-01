.TITLE	StarLet POP3 Server - message file
.IDENT	'VERSION 1.0'

.facility	POP3,1  /prefix=POP3_

.severity	fatal	
ERRTHREAD	<Could not create a processor thread,errno=!AZ.>/FAO=1
GENERR		<General error is occured, status=%x!XL.>/FAO=1
VMSHUT		<Error closing VMS context, status=%x!XL.>/FAO=1
ILLCMD		<Illegal/urecognized command line '!AS'.>/FAO=1
NOFREECTX	<No free context>

.severity	informational
OPENED		<Connection is opened.>
SOPENED		<Connection/session has been opened, status=%x!XL.>/FAO=1
CLOSED		<Connection/session has been closed, status=%x!XL.>/FAO=1
DEBUG		<'!AS'>/FAO=1
EXPIRED		<Message #!UL has been expired (From: !AC, To: !AC, Subj: !AC, Date: !%D).>/FAO=5
STARTED		<StarLet POP3 Server is up on port !UL, Mail expiration=!ULd, login interval=!ULs, I/O timeout=!%T.>/FAO=4
INIMAILCTX	<VMS Mail box: total/new=!UL/!UL msgs.>/FAO=2
RECLAIM		<Buckets: Data scanned/reclaimed=!UL/!UL, Index reclaimed=!UL, Total reclaimed=!UL, status=%x!XL.>/FAO=5
SHUTMAILCTX	<VMS Mail box: deleted/old/sent=!UL/!UL/!UL msgs, deleted !UL bytes.>/FAO=4
MDELETE		<Message #!UL has been deleted, status=%x!XL.>/FAO=2
SENTSHUT	<Sent shutdown signal to !AS, status=%x!XL.>/FAO=2
.end
