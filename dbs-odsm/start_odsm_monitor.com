$ ! Procedure:	START_ODSM_MONITOR.COM
$ __vfy = "VFY_''f$parse(f$environment("procedure"),,,"name")'"
$ if (f$type('__vfy') .eqs. "") then __vfy = 0
$ __vfy_saved = f$verify(&__vfy)
$ set noon
$ define/system/nolog odsm_exe wop$system
$ define/system/nolog odsm_data wop$system
$ run/detached sys$system:loginout/authorize -
		/input=odsm_exe:odsm_monitor.com -
		/output=sys$manager:odsm_monitor.log -
		/error=nla0:
$ !'f$verify(__vfy_saved)'
$ exitt 1
$ !+==========================================================================
$ !
$ ! Procedure:	START_ODSM_MONITOR.COM
$ !
$ ! Purpose:	To start the ODSM Monitor as a detached process.
$ !
$ ! Parameters:
$ !
$ ! History:
$ !		14-Sep-1993, DBS; Version V1-001
$ !	001 -	Original version.
$ !-==========================================================================
