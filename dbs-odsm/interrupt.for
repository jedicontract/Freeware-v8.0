	program interrupt

C+
C Version:	X1-005
C
C Facility:	General Utilities
C
C Abstract:	This program will write a message to a mailbox.
C		Designed for locally written detached processors that use
C		a standard communication interface.
C
C Environment:	VMS V5.0 and later.
C
C	module interrupt_cld
C
C	define verb interrupt
C		qualifier vanilla, nonnegatable
C		qualifier nodefault, nonnegatable
C		qualifier noprefix, nonnegatable
C		parameter p1, label=utility, prompt="whom"
C				,value (required)
C		parameter p2, label=message, prompt="message"
C				,value (required, type=$rest_of_line)
C
C History:
C
C	14-Dec-1989, DBS; Version X1-001
C 001 -	Original version.
C	07-Feb-1990, DBS; Version X1-002
C 002 -	Catch a ^Z on the get_foreign prompt and don't let cli ask again.
C	26-Feb-1990, DBS; Version X1-003
C 003 -	Fix missing argument to lib$signal() call.
C	12-Jul-1990, DBS; Version X1-004
C 004 -	Prefixed the message with a char(130) to make it more difficult to
C	send bogus messages.
C	03-Nov-1993, DBS; Version X1-005
C 005 -	Added a /noprefix qualifier for use with other detached processes
C	and added /vanilla to imply /nodefault and /noprefix (saves typing).
C-

 	implicit none

	include '($CLIDEF)/nolist'
	include '($IODEF)/nolist'
	include '($RMSDEF)/nolist'

C External references

	integer*4	cli$dcl_parse
	external	cli$dcl_parse
	integer*4	cli$get_value
	external	cli$get_value
	integer*4	cli$present
	external	cli$present
	external	interrupt_cld
	external	intrupt__nomailbox
	external	intrupt__notdeliv
	external	intrupt__success
	integer*4	lib$get_foreign
	external	lib$get_foreign
	external	lib$get_input
	external	lib$put_output
	external	lib$signal
	external	str_collapse
	integer*4	str_len
	external	str_len
	external	str_uppercase
	integer*4	sys$assign
	external	sys$assign
	integer*4	sys$qiow
	external	sys$qiow

C Type declarations for variables

	integer*4	do, nothing
	integer*4	istatus
	integer*2	mbx_channel
	character	mbx_name*256
	integer*4	mbx_name_len
	integer*4	mbx_status
	character	mbx_suffix*(*)
	parameter	(mbx_suffix = '_MAILBOX')
	integer*2	qio_iosb(4)

	integer*4	cli_status
	character	cli_c_verb*9		/'INTERRUPT'/
	character	cli_c_prompt*7		/'_whom: '/
	character	cli_t_command*256
	integer*4	cli_s_command
	integer*4	cli_nodefault
	character	cli_c_nodefault*9	/'NODEFAULT'/
	integer*4	cli_noprefix
	character	cli_c_noprefix*8	/'NOPREFIX'/
	integer*4	cli_vanilla
	character	cli_c_vanilla*7		/'VANILLA'/
	character	cli_c_utility*7		/'UTILITY'/
	integer*2	cli_s_utility
	character	cli_t_utility*80
	character	cli_c_message*7		/'MESSAGE'/
	integer*2	cli_s_message
	character	cli_t_message*256

C+
C Mainline
C-

	istatus = lib$get_foreign (cli_t_command, cli_c_prompt, cli_s_command)

	if (istatus .eq. rms$_eof) then
		do = nothing
	else
	cli_t_command = cli_c_verb//' '//cli_t_command
	cli_s_command = cli_s_command + len(cli_c_verb) + 1

	call cli$dcl_parse (cli_t_command, interrupt_cld
	1			,lib$get_input, lib$get_input
	1			,cli_c_prompt)

	cli_nodefault = cli$present (cli_c_nodefault)
	cli_noprefix  = cli$present (cli_c_noprefix)
	cli_vanilla   = cli$present (cli_c_vanilla)

	if (cli_vanilla) then
		cli_nodefault = cli_vanilla
		cli_noprefix  = cli_vanilla
	endif !(cli_vanilla) then

	call cli$get_value (cli_c_utility, cli_t_utility, cli_s_utility)

	if (cli_s_utility .eq. 0) then
		do = nothing
	else
	call cli$get_value (cli_c_message, cli_t_message, cli_s_message)

	if (cli_s_message .eq. 0) then
		do = nothing
	else
	if (cli_nodefault) then
		mbx_name = cli_t_utility(1:cli_s_utility)
	else
	mbx_name = cli_t_utility(1:cli_s_utility)//mbx_suffix
	endif !(cli_nodefault) then

	call str_collapse (mbx_name, mbx_name)
	call str_uppercase (mbx_name)

	mbx_name_len = str_len (mbx_name)

	mbx_status = sys$assign (%descr(mbx_name(1:mbx_name_len))
	1			,%ref(mbx_channel),,)

	if (.not. mbx_status) then
		call lib$signal (intrupt__nomailbox
	1			,%val(1), %descr(mbx_name(1:mbx_name_len))
	1			,%val(mbx_status))
	else
	if (cli_s_message .ne. 0) then
		if (.not. cli_noprefix) then
		  cli_t_message = char(130)//cli_t_message(1:cli_s_message)
		  cli_s_message = cli_s_message + 1
		endif !(.not. cli_noprefix) then
		mbx_status = sys$qiow (,%val(mbx_channel)
	1				,%val(io$_writevblk .or. io$m_now)
	1				,%ref(qio_iosb(1)),,
	1				,%ref(cli_t_message)
	1				,%val(cli_s_message),,,,)
		if (.not. mbx_status) then
			call lib$signal (intrupt__notdeliv
	1				,%val(0)
	1				,%val(mbx_status))
		else
		call lib$signal (intrupt__success
	1			,%val(1), %descr(mbx_name(1:mbx_name_len)))
		endif !(.not. mbx_status) then
	endif !(cli_s_message .ne. 0) then
	endif !(.not. mbx_status) then
	endif !(cli_s_message .eq. 0) then
	endif !(cli_s_utility .eq. 0) then
	endif !(istatus .eq. rms$_eof) then

	end
