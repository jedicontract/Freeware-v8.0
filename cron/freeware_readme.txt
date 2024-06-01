CRON, SYSTEM_MGMT, VMS port of CRON utlity

This is a VMS port of the CRON utility.

I did NOT make the port, but just added some DCL code around it
and made some minor fixes to the sources and packaged it as a
PCSI kit.

The kit is installed using PCSI and should NOT be installed on the
system disk, because the user specific spool files and the logfiles
are created under its root.

The kit creates an identifier CRON_USER, which is used to control access
to the SPOOL subdirectory and should be granted to all users, that should
be able to submit CRON jobs.

To use CRON, a user should define a foreign command like:

$ CLONTAB == "@CRON$LIB:[000000]CRONTAB"

This is a wrapper for the CLONTAB utility (sorry its help is german), but
CLONTAB.EXE can be called directly also with:

$ CLONTAB == "$CRON$LIB:[000000]CLONTAB.EXE_ALPHA" ! for alphas...

The file CRON_STARTUP can be used to STOP or START the CRON daemon like
'$ @CRON_STARTUP STOP' and '$ @CRON_STARRTUP START' and should be included
in the startup/shutdown procedures. CRON is started by this procedure on
standalone systems and voting cluster nodes. On satellites only the logical
names CRON$LIB and CRON_DIR are defined. 

The kit consists of the following files:

FREEWARE_README.TXT		- this readme
FREEWARE_REALEASE.TXT|PDF	- freeware release form
BUILD.COM 			- routine to compile/link from sources
CLONTAB.C 			- source file for CLONTAB
CLONTAB.EXE_ALPHA 		- ALPHA executable
CLONTAB.EXE_VAX 		- VAX executable
CLONTAB.OBJ_ALPHA 		- ALPHA object
CLONTAB.OBJ_ALPHA 		- VAX object
CLONTAB.TXT 			- help file for CLONTAB in english
CRON.HLP 			- (german) helep module for HELPLIB
CRON013.RELEASE_NOTES 		- releasenotes (german)
CRONTAB.COM 			- wrapper routine for CLONTAB
CRON_POSTINSTALL.COM 		- called via PCSI to setup logicals
CRON_STARTUP.COM 		- CRON start/stop routine
MYCRON.C 			- source file for MYCRON (the daemon)
MYCRON.H 			- include file for MYCRON
MYCRON.EXE_ALPHA 		- ALPHA executable
MYCRON.EXE_VAX 			- VAX executable
MYCRON.OBJ_ALPHA 		- ALPHA object
MYCRON.OBJ_VAX 			- VAX object
MYCRON_STARTUP.COM 		- startup routine for detached process
SEND_MAIL.COM 			- internal mail routine

The executables are compiled and linked under OpenVMS/Alpha V8.2 und
OpenVMS/VAX V7.3.

Sorry, I have no IA64 system at hand, so I could not test the stuff on IA64 or
create executables.

If you encounter any errors with this kit or have some suggestions
for improvement, please send mail to:

	karl.rohwedder(at)gmx.de

